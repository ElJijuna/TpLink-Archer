import contextlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).parents[1] / 'tools'))
import sequence_epoch as e


def evidence():
    return ([{'phase': 'epoch_mac_reset_verified'}, {'phase': 'cpu_reset'}]
            + [{'operation': 'firmware_bulk_write', 'status': 0}] * 212
            + [{'phase': 'firmware_ready'}, {'operation': 'bulk_async_drained', 'pending': 0},
               {'result': 'epoch_firmware_reloaded_no_h2c', 'h2c_submissions': 0, 'quiet_ms_min': 250}])


class EpochTest(unittest.TestCase):
    def test_rejects_incomplete_boundary(self):
        self.assertEqual(e.validate_reload(evidence())['h2c_submissions'], 0)
        cases = [evidence()[:-1], evidence()[1:], list(reversed(evidence()))]
        dirty = evidence();dirty[-1] = dict(dirty[-1], h2c_submissions=1);cases.append(dirty)
        pending = evidence();pending[-2] = dict(pending[-2], pending=1);cases.append(pending)
        for case in cases:
            with self.assertRaises(ValueError):
                e.validate_reload(case)

    def run_transition(self, fail, dirty=False, cold=False):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp);(root / 'artifacts').mkdir();(root / 'build').mkdir()
            ledger = root / 'artifacts/37ad-0104-sequences.bin'
            previous = b'ARCHSEQ\x01' + bytes([1] * 245 + [0] * 11)
            ledger.write_bytes(previous)
            (root / 'artifacts/37ad-0104-context-reservation').write_text('DIRTY\n' if dirty else 'CLEAN\n')
            helper = root / 'build/archer-epoch-boot-universal'
            rows=evidence()
            if cold:rows[0]={'phase':'epoch_cold_profile_verified'}
            helper.write_text('#!/usr/bin/env python3\n' + ('raise SystemExit(7)\n' if fail else 'print(' + repr('\n'.join(json.dumps(x) for x in rows)) + ')\n'))
            helper.chmod(0o700)
            output = root / 'attempt'
            with patch.multiple(e, ROOT=root, LEDGER=ledger), contextlib.redirect_stdout(io.StringIO()):
                if fail:
                    with self.assertRaises(RuntimeError):
                        e.transition(output, discard_dirty_context=dirty, cold=cold)
                else:
                    e.transition(output, discard_dirty_context=dirty, cold=cold)
            self.assertEqual((output/'previous-context.bin').read_bytes(), b'DIRTY\n' if dirty else b'CLEAN\n')
            self.assertEqual((root/'artifacts/37ad-0104-context-reservation').read_bytes(), b'DIRTY\n' if dirty and fail else b'CLEAN\n')
            self.assertEqual((output / 'previous-ledger.bin').read_bytes(), previous)
            self.assertEqual(ledger.read_bytes(), previous if fail else e.new_epoch_bytes())
            self.assertEqual(Path(str(ledger) + '.epoch-transition').read_text(), 'DIRTY\n' if fail else 'CLEAN\n')
            self.assertEqual(json.loads((output / 'epoch.json').read_text())['status'], 'stopped' if fail else 'committed')

    def test_reload_failure_preserves_old_ledger_and_fence(self):
        self.run_transition(True)

    def test_verified_reload_archives_and_commits(self):
        self.run_transition(False)

    def test_dangling_fence_blocks_before_reload(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp);ledger = root / 'ledger'
            ledger.write_bytes(b'ARCHSEQ\x01' + bytes([1, 1]) + bytes(254))
            Path(str(ledger) + '.epoch-transition').symlink_to(root / 'absent')
            with patch.multiple(e, ROOT=root, LEDGER=ledger), patch.object(e.subprocess, 'run') as launch:
                with self.assertRaises(ValueError):
                    e.transition(root / 'attempt')
                launch.assert_not_called()

    def test_dirty_context_recovery_requires_verified_boundary(self):
        self.run_transition(True, dirty=True)
        self.run_transition(False, dirty=True)

    def test_dirty_context_default_refuses_before_child(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);(root/'artifacts').mkdir()
            ledger=root/'ledger';ledger.write_bytes(e.new_epoch_bytes())
            context=root/'artifacts/37ad-0104-context-reservation'
            context.write_bytes(b'DIRTY\n')
            with patch.multiple(e,ROOT=root,LEDGER=ledger),patch.object(e.subprocess,'run') as launch:
                with self.assertRaises(ValueError):e.transition(root/'attempt')
                launch.assert_not_called()
            self.assertEqual(context.read_bytes(),b'DIRTY\n')
            self.assertEqual(ledger.read_bytes(),e.new_epoch_bytes())

    def test_cold_boundary_is_distinct_and_transactional(self):
        rows=evidence();rows[0]={'phase':'epoch_cold_profile_verified'}
        self.assertEqual(e.validate_reload(rows,cold=True)['h2c_submissions'],0)
        with self.assertRaises(ValueError):e.validate_reload(rows)
        with self.assertRaises(ValueError):e.validate_reload(evidence(),cold=True)
        self.run_transition(False,dirty=True,cold=True)
        self.run_transition(True,dirty=True,cold=True)

    def test_finish_preserved_cold_download_does_not_reload(self):
        for fail_finish in (False,True):
            with self.subTest(fail_finish=fail_finish),tempfile.TemporaryDirectory() as tmp:
                root=Path(tmp);(root/'artifacts').mkdir();(root/'build').mkdir()
                ledger=root/'artifacts/37ad-0104-sequences.bin'
                old=b'ARCHSEQ\x01'+bytes([1]*50+[0]*206);ledger.write_bytes(old)
                context=root/'artifacts/37ad-0104-context-reservation';context.write_bytes(b'DIRTY\n')
                helper=root/'build/archer-epoch-boot-universal'
                partial=evidence()[:-2];partial[0]={'phase':'epoch_cold_profile_verified'}
                helper.write_text('#!/usr/bin/env python3\nprint('+repr('\n'.join(json.dumps(x) for x in partial))+')\nraise SystemExit(7)\n');helper.chmod(0o700)
                output=root/'attempt'
                with patch.multiple(e,ROOT=root,LEDGER=ledger),contextlib.redirect_stdout(io.StringIO()):
                    with self.assertRaises(RuntimeError):e.transition(output,True,True)
                    initial_log=(output/'reload.jsonl').read_bytes()
                    final=evidence()[-2:]
                    helper.write_text('#!/usr/bin/env python3\nimport sys\nfrom pathlib import Path\nPath(sys.argv[2]).mkdir()\n'+('raise SystemExit(7)\n' if fail_finish else 'print('+repr('\n'.join(json.dumps(x) for x in final))+')\n'))
                    if fail_finish:
                        with self.assertRaises(RuntimeError):e.transition(output,True,finish_cold=True)
                    else:e.transition(output,True,finish_cold=True)
                self.assertEqual((output/'reload.jsonl').read_bytes(),initial_log)
                self.assertEqual(ledger.read_bytes(),old if fail_finish else e.new_epoch_bytes())
                self.assertEqual(context.read_bytes(),b'DIRTY\n' if fail_finish else b'CLEAN\n')
                self.assertEqual(Path(str(ledger)+'.epoch-transition').read_bytes(),b'DIRTY\n' if fail_finish else b'CLEAN\n')

class ColdPreflightResumeTest(unittest.TestCase):
    def test_real_preflight_rejects_any_boundary_or_extra_write(self):
        rows=[json.loads(x) for x in (Path(__file__).parent/'fixtures/cold-preflight-rejected-stage107.jsonl').read_text().splitlines()]
        record=dict(status='stopped',mode='cold-reload-no-h2c',reload_exit=2)
        stderr='epoch-boot: Cold BB reset profile absent; keep transition fenced, no replay\n'
        e.validate_cold_preflight_stop(record,rows,stderr)
        for extra in [dict(phase='cpu_reset'),dict(operation='firmware_bulk_write',status=0),dict(operation='write',address=0x84,width=4,completed_bytes=4,status=0,value=0)]:
            with self.assertRaises(ValueError):e.validate_cold_preflight_stop(record,rows+[extra],stderr)
        for changed in [dict(record,resuming_cold_preflight=True),dict(record,reload_exit=0)]:
            with self.assertRaises(ValueError):e.validate_cold_preflight_stop(changed,rows,stderr)

    def test_resume_preserves_original_and_commits_only_after_boundary(self):
        for fail,collision in [(False,False),(True,False),(False,True),(True,True)]:
            with self.subTest(fail=fail,collision=collision),tempfile.TemporaryDirectory() as tmp:
                root=Path(tmp);(root/'artifacts').mkdir();(root/'build').mkdir()
                ledger=root/'artifacts/37ad-0104-sequences.bin';old=b'ARCHSEQ\x01'+bytes([1])*113+bytes(143);ledger.write_bytes(old)
                context=root/'artifacts/37ad-0104-context-reservation';context.write_bytes(b'DIRTY\n')
                helper=root/'build/archer-epoch-boot-universal'
                raw=(Path(__file__).parent/'fixtures/cold-preflight-rejected-stage107.jsonl').read_text()
                stderr='epoch-boot: Cold BB reset profile absent; keep transition fenced, no replay\n'
                helper.write_text('#!/usr/bin/env python3\nimport sys\nfrom pathlib import Path\nPath(sys.argv[2]).mkdir()\nsys.stdout.write('+repr(raw)+')\nsys.stderr.write('+repr(stderr)+')\nsys.exit(2)\n');helper.chmod(0o700)
                output=root/'attempt'
                with patch.multiple(e,ROOT=root,LEDGER=ledger),contextlib.redirect_stdout(io.StringIO()):
                    with self.assertRaises(RuntimeError):e.transition(output,True,True)
                    if collision:
                        original=json.loads((output/'epoch.json').read_text());e.save(output/'preflight-stop.json',original)
                        (output/'resume-cold.jsonl').write_bytes(b'');(output/'resume-cold.stderr').write_text('epoch-boot: New epoch capture directory required; keep transition fenced, no replay\n')
                        original.update(resuming_cold_preflight=True,boundary_stdout_file='resume-cold.jsonl',preflight_stop_sha256=e.digest(output/'preflight-stop.json'),reload_stdout_sha256=e.digest(output/'resume-cold.jsonl'),reload_stderr_sha256=e.digest(output/'resume-cold.stderr'))
                        e.save(output/'epoch.json',original)
                    rows=evidence();rows[0]={'phase':'epoch_cold_profile_verified'}
                    helper.write_text('#!/usr/bin/env python3\nimport sys\nfrom pathlib import Path\nPath(sys.argv[2]).mkdir()\n'+('raise SystemExit(7)\n' if fail else 'print('+repr('\n'.join(json.dumps(x) for x in rows))+')\n'))
                    if fail:
                        with self.assertRaises(RuntimeError):e.transition(output,True,resume_cold_preflight=True)
                    else:e.transition(output,True,resume_cold_preflight=True)
                self.assertEqual((output/'reload.jsonl').read_text(),raw)
                self.assertEqual((output/'reload.stderr').read_text(),stderr)
                self.assertEqual((output/'previous-ledger.bin').read_bytes(),old)
                self.assertEqual(ledger.read_bytes(),old if fail else e.new_epoch_bytes())
                self.assertEqual(context.read_bytes(),b'DIRTY\n' if fail else b'CLEAN\n')
                self.assertEqual(Path(str(ledger)+'.epoch-transition').read_bytes(),b'DIRTY\n' if fail else b'CLEAN\n')
