import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).parents[1]/'tools'))
import special_probe_attempt as p


def calibration():
    return dict(stage=84,channel=9,frequency_mhz=2452,mcc_slot=0,
        terminal_success_count=8,context='CLEAN',network_tx_paused=True,
        available_sequences=224,new_network_frame_submissions=0,
        commands=[dict(sequence=s,rfk_command=c) for s,c in zip(range(24,32),[8,0,8,4,1,0,3,6])],
        rxdck_replay=dict(rxdck_clean=True,channels=[9,9],timeouts=[0,0]),evidence={'dummy':'sha'})


class SpecialProbeTest(unittest.TestCase):
    def test_rejects_wrong_channel_history_and_report_tag(self):
        p.validate_calibration(calibration())
        for change in (dict(channel=1),dict(commands=[]),dict(context='DIRTY')):
            with self.assertRaises(ValueError):p.validate_calibration({**calibration(),**change})
        rows=[dict(operation='single_probe_usb_write',endpoint=5,bytes=104,status=0),
              dict(operation='single_probe_tx_report',tag=2,state=0,tx_count=1),
              dict(result='context_cleanup_acknowledged',network_tx_paused=True),
              dict(operation='bulk_async_drained',pending=0)]
        p.validate_result(rows)
        rows[1]['tag']=3
        p.validate_result(rows,3)
        with self.assertRaises(ValueError):p.validate_result(rows)
        rows[1]['tag']=1
        with self.assertRaises(ValueError):p.validate_result(rows)

    def test_failed_child_cannot_repeat_and_preserves_old_marker(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t).resolve()
            for name in ('artifacts','research','build'):(root/name).mkdir()
            ledger=root/'artifacts/37ad-0104-sequences.bin'
            ledger.write_bytes(b'ARCHSEQ\x01'+bytes([1])*32+bytes(224))
            Path(str(ledger)+'.epoch-transition').write_bytes(b'CLEAN\n')
            (root/'artifacts/37ad-0104-context-reservation').write_bytes(b'CLEAN\n')
            old=root/'artifacts/37ad-0104-single-probe-attempted';old.write_bytes(b'OLD RESERVED\n')
            raw=root/'artifacts/evidence';raw.write_bytes(b'captured')
            record=calibration();record['evidence']={'artifacts/evidence':p.digest(raw)}
            (root/'research/channel9-calibration-stage84-observation.json').write_text(json.dumps(record))
            helper=root/'build/archer-special-probe-universal'
            helper.write_text('#!/usr/bin/env python3\nraise SystemExit(7)\n');helper.chmod(0o700)
            with patch.object(p,'ROOT',root),patch.object(p,'LEDGER',ledger):
                with self.assertRaises(RuntimeError):p.execute(root/'first')
                with self.assertRaises(FileExistsError):p.execute(root/'second')
            self.assertEqual(old.read_bytes(),b'OLD RESERVED\n')
            self.assertEqual((root/'artifacts/37ad-0104-special-channel9-probe-attempted').read_bytes(),b'RECOVERY_RESERVED\n')
