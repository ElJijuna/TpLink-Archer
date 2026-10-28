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

    def run_transition(self, fail):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp);(root / 'artifacts').mkdir();(root / 'build').mkdir()
            ledger = root / 'artifacts/37ad-0104-sequences.bin'
            previous = b'ARCHSEQ\x01' + bytes([1] * 245 + [0] * 11)
            ledger.write_bytes(previous)
            (root / 'artifacts/37ad-0104-context-reservation').write_text('CLEAN\n')
            helper = root / 'build/archer-epoch-boot-universal'
            helper.write_text('#!/usr/bin/env python3\n' + ('raise SystemExit(7)\n' if fail else 'print(' + repr('\n'.join(json.dumps(x) for x in evidence())) + ')\n'))
            helper.chmod(0o700)
            output = root / 'attempt'
            with patch.multiple(e, ROOT=root, LEDGER=ledger), contextlib.redirect_stdout(io.StringIO()):
                if fail:
                    with self.assertRaises(RuntimeError):
                        e.transition(output)
                else:
                    e.transition(output)
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
