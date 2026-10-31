import copy
import json
from pathlib import Path
import sys
import unittest
import tempfile
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).parents[1]/'tools'))
import finish_probe_cleanup as p


class CleanupProofTest(unittest.TestCase):
    def test_terminal_failure_is_distinct_from_unknown_transport(self):
        record=dict(stage=86,context='DIRTY',attempt=dict(status='stopped',exit_code=2,tag=3,unicast=True),
            tx_reports=[dict(operation='single_probe_tx_report',tag=3,state=1,tx_count=1)],
            cleanup_steps_6_7_8_acknowledged=True,callbacks_drained=True,evidence={'raw':'sha'},
            last_scheduler_read=dict(operation='read',address=0x10398,status=0,completed_bytes=4,value=0))
        p.validate_observation(record)
        for key,value in [('tx_reports',[]),('cleanup_steps_6_7_8_acknowledged',False),
                          ('callbacks_drained',False),('evidence',{}),('stage',85)]:
            bad=copy.deepcopy(record);bad[key]=value
            with self.assertRaises(ValueError):p.validate_observation(bad)
        for field,value in [('value',0x100),('status',-1),('completed_bytes',0)]:
            bad=copy.deepcopy(record);bad['last_scheduler_read'][field]=value
            with self.assertRaises(ValueError):p.validate_observation(bad)


    def test_live_verification_failure_preserves_dirty_and_ledger(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t).resolve()
            for d in ('artifacts','research','build'):(root/d).mkdir()
            ledger=root/'artifacts/37ad-0104-sequences.bin';ledger.write_bytes(b'original ledger')
            Path(str(ledger)+'.epoch-transition').write_bytes(b'CLEAN\n')
            context=root/'artifacts/37ad-0104-context-reservation';context.write_bytes(b'DIRTY\n')
            raw=root/'artifacts/evidence';raw.write_bytes(b'original capture')
            record=dict(stage=86,context='DIRTY',attempt=dict(status='stopped',exit_code=2,tag=3,unicast=True,ledger_after_sha256=p.digest(ledger)),
                tx_reports=[dict(operation='single_probe_tx_report',tag=3,state=1,tx_count=1)],
                cleanup_steps_6_7_8_acknowledged=True,callbacks_drained=True,
                evidence={'artifacts/evidence':p.digest(raw)},
                last_scheduler_read=dict(operation='read',address=0x10398,status=0,completed_bytes=4,value=0))
            (root/'research/special-unicast-stage86-observation.json').write_text(json.dumps(record))
            replay=root/'build/archer-context-cleanup-replay-universal'
            proof=dict(result='stage86_cleanup_replayed',issued_commands=9,final_step=9,retry_limit_terminal=True,pending_h2c=False)
            replay.write_text('#!/usr/bin/env python3\nprint('+repr(json.dumps(proof))+')\n');replay.chmod(0o700)
            verify=root/'build/archer-cleanup-verify-universal'
            verify.write_text('#!/usr/bin/env python3\nraise SystemExit(7)\n');verify.chmod(0o700)
            with patch.object(p,'ROOT',root),patch.object(p,'LEDGER',ledger):
                with self.assertRaises(RuntimeError):p.execute(root/'failed')
            self.assertEqual(context.read_bytes(),b'DIRTY\n')
            self.assertEqual(ledger.read_bytes(),b'original ledger')
            self.assertEqual(json.loads((root/'failed/cleanup.json').read_text())['status'],'stopped')

    @unittest.skipUnless(Path('build/archer-context-cleanup-replay-universal').exists() and
                         Path('artifacts/special-unicast-probe-stage86/capture').exists(),
                         'Requires local stage86 captures and replay binary')
    def test_raw_replay_rejects_missing_final_receive_ack_identity(self):
        import shutil
        import subprocess
        helper=Path('build/archer-context-cleanup-replay-universal').resolve()
        source=Path('artifacts/special-unicast-probe-stage86/capture')
        good=subprocess.run([str(helper),'--stage86',str(source)],capture_output=True)
        self.assertEqual(good.returncode,0,good.stderr)
        with tempfile.TemporaryDirectory() as t:
            capture=Path(t)/'capture';shutil.copytree(source,capture)
            # MAC INFO REC_ACK for category1,class9,function0x28,sequence49.
            # Change only its sequence, leaving USB framing/length valid.
            identity=bytes.fromhex('25283100');changed=0
            for path in capture.glob('rx-*.bin'):
                data=path.read_bytes()
                if identity in data:
                    changed+=data.count(identity)
                    path.write_bytes(data.replace(identity,bytes.fromhex('25283200')))
            self.assertEqual(changed,1)
            bad=subprocess.run([str(helper),'--stage86',str(capture)],capture_output=True)
            self.assertNotEqual(bad.returncode,0)
            self.assertIn(b'Incomplete raw cleanup proof',bad.stderr)
