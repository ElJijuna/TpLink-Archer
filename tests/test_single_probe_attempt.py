import contextlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).parents[1]/'tools'))
import single_probe_attempt as p


def cold():
    names=['inspect','boot','radio','initial_rfk','tx_rfk','scan','sweep']
    terminals=['firmware_controls','firmware_ready','scc_radio_nctl_applied',
               'initial_pre_dack_rxdck_clean','tx_rfk_commands_completed',
               'context_cleanup_acknowledged','passive_channel1_6_11_1_complete']
    return dict(status='completed',mode='cold-start',
                phases=[dict(name=n,status='completed',exit_code=0,result=dict(result=t,network_tx_paused=True))
                        for n,t in zip(names,terminals)],sweep_report=dict(live_equals_replay=True))


class ProbeAttemptTest(unittest.TestCase):
    def test_cold_evidence_required(self):
        p.validate_cold(cold())
        for update in [dict(mode='resume-bootstrap'),dict(status='stopped'),dict(phases=[])]:
            with self.assertRaises(ValueError):p.validate_cold({**cold(),**update})
        r=cold();r['phases'][3]['result']={}
        with self.assertRaises(ValueError):p.validate_cold(r)

    def test_failed_child_keeps_reservation_and_blocks_replay(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);(root/'artifacts').mkdir();(root/'build').mkdir()
            ledger=root/'artifacts/37ad-0104-sequences.bin';ledger.write_bytes(b'ARCHSEQ\x01'+bytes([1,1])+bytes(254))
            Path(str(ledger)+'.epoch-transition').write_bytes(b'CLEAN\n')
            (root/'artifacts/37ad-0104-context-reservation').write_bytes(b'CLEAN\n')
            marker=root/'artifacts/37ad-0104-single-probe-attempted';marker.write_bytes(b'TAG1 RESERVED\n')
            helper=root/'build/archer-single-probe-universal';helper.write_text('#!/usr/bin/env python3\nraise SystemExit(7)\n');helper.chmod(0o700)
            history=root/'cold';history.mkdir();r=cold();r['ledger_sha256_after']=p.digest(ledger)
            (history/'workflow.json').write_text(json.dumps(r))
            with patch.multiple(p,ROOT=root,LEDGER=ledger),contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaises(RuntimeError):p.execute(history,root/'attempt')
                with self.assertRaises(ValueError):p.execute(history,root/'replay')
            self.assertEqual(marker.read_bytes(),b'RECOVERY_RESERVED\n')
            self.assertEqual((root/'attempt/previous-probe-marker.bin').read_bytes(),b'TAG1 RESERVED\n')
            self.assertEqual(json.loads((root/'attempt/attempt.json').read_text())['exit_code'],7)

    def test_probe_requires_real_terminal_evidence(self):
        rows=[dict(operation='single_probe_usb_write',endpoint=5,bytes=96,status=0),
              dict(operation='single_probe_tx_report',tag=1,state=0,tx_count=1),
              dict(result='context_cleanup_acknowledged',network_tx_paused=True),
              dict(operation='bulk_async_drained',pending=0)]
        self.assertTrue(p.validate_probe(rows)['firmware_tx_done'])
        for bad in [rows[:-1],rows+[rows[0]],[rows[0],{**rows[1],'state':1},*rows[2:]]]:
            with self.assertRaises(ValueError):p.validate_probe(bad)
