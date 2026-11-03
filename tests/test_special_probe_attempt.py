import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).parents[1]/'tools'))
import special_probe_attempt as p
from external_probe_capture import validate_capture_header, validate_live_decode


def calibration():
    return dict(stage=84,channel=9,frequency_mhz=2452,mcc_slot=0,
        terminal_success_count=8,context='CLEAN',network_tx_paused=True,
        available_sequences=224,new_network_frame_submissions=0,
        commands=[dict(sequence=s,rfk_command=c) for s,c in zip(range(24,32),[8,0,8,4,1,0,3,6])],
        rxdck_replay=dict(rxdck_clean=True,channels=[9,9],timeouts=[0,0]),evidence={'dummy':'sha'})


class SpecialProbeTest(unittest.TestCase):
    def test_tag7_requires_tag6_cleanup_and_repeated_crc_validation(self):
        previous=dict(stage=96,context='CLEAN',remaining_sequences=197,pcap_sha256='pcap',
            tx_reports=[dict(operation='single_probe_tx_report',tag=6,state=1,tx_count=1)],
            cleanup=[dict(network_tx_paused=True)])
        validation=dict(stage=98,context='CLEAN',remaining_sequences=197,
            state=dict(all_state_unchanged=True),validation=dict(result='validated',repeated_snapshots=3,stable_registers=210),
            external_crc=dict(source_sha256='pcap',valid_target_beacons=381,valid_test_mac_frames=0))
        p.validate_phy_history(previous,validation)
        for change in (dict(context='DIRTY'),dict(remaining_sequences=196),dict(tx_reports=[]),dict(cleanup=[])):
            with self.assertRaises(ValueError):p.validate_phy_history({**previous,**change},validation)
        for change in (dict(state={}),dict(validation={}),dict(external_crc=dict(source_sha256='wrong'))):
            with self.assertRaises(ValueError):p.validate_phy_history(previous,{**validation,**change})

    def test_phy_measurement_is_paired_bounded_and_not_air_proof(self):
        record=dict(operation='probe_phy_tx_counters',before=[65535,0,0,0],after=[0,1,0,0],
            modulo_delta=[1,1,0,0],controls_enabled=True,antenna_emission_proven=False)
        p.validate_phy_measurement([record])
        for rows in ([],[record,record],[{**record,'after':[65536,1,0,0]}],
                     [{**record,'modulo_delta':[0,1,0,0]}],[{**record,'controls_enabled':False}]):
            with self.assertRaises(ValueError):p.validate_phy_measurement(rows)

    def test_external_capture_requires_clean_terminal_history_and_radiotap(self):
        previous=dict(stage=92,context='CLEAN',remaining_sequences=206,
            tx_reports=[dict(operation='single_probe_tx_report',tag=5,state=1,tx_count=1)],
            cleanup=dict(network_tx_paused=True))
        baseline=dict(stage=94,frequencies_mhz={'2452':1499},
            target_beacons_without_bad_fcs_flag=165,archer_tx_submissions=0,capture_exit=0)
        p.validate_external_history(previous,baseline)
        for change in (dict(context='DIRTY'),dict(tx_reports=[]),dict(remaining_sequences=205)):
            with self.assertRaises(ValueError):p.validate_external_history({**previous,**change},baseline)
        for change in (dict(frequencies_mhz={'5180':1499}),dict(target_beacons_without_bad_fcs_flag=0),dict(archer_tx_submissions=1)):
            with self.assertRaises(ValueError):p.validate_external_history(previous,{**baseline,**change})
        header=b'\xd4\xc3\xb2\xa1'+bytes(16)+(127).to_bytes(4,'little')
        validate_capture_header(header)
        for bad in (header[:20],header[:20]+(1).to_bytes(4,'little')):
            with self.assertRaises(ValueError):validate_capture_header(bad)
        with self.assertRaises(ValueError):p.execute(Path('unused'),external_capture=True)
        beacon='1.0 Mb/s 2452 MHz BSSID:50:92:6a:11:06:66 Beacon (Especial)'
        validate_live_decode(beacon)
        for text in ('',beacon.replace('2452','5180'),'bad-fcs '+beacon,beacon+'\nmetadata absent'):
            with self.assertRaises(ValueError):validate_live_decode(text)

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

    def test_external_handoff_blocks_without_marker_and_consumes_only_new_marker(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t).resolve()
            for name in ('artifacts','research','build'):(root/name).mkdir()
            ledger=root/'artifacts/37ad-0104-sequences.bin'
            ledger.write_bytes(b'ARCHSEQ\x01'+bytes([1])*50+bytes(206))
            Path(str(ledger)+'.epoch-transition').write_bytes(b'CLEAN\n')
            (root/'artifacts/37ad-0104-context-reservation').write_bytes(b'CLEAN\n')
            old=root/'artifacts/37ad-0104-special-channel9-ack-probe-attempted';old.write_bytes(b'OLD\n')
            raw=root/'artifacts/raw';raw.write_bytes(b'raw')
            calibration_record=calibration();calibration_record['evidence']={'artifacts/raw':p.digest(raw)}
            (root/'research/channel9-calibration-stage84-observation.json').write_text(json.dumps(calibration_record))
            previous=dict(stage=92,context='CLEAN',remaining_sequences=206,
                tx_reports=[dict(operation='single_probe_tx_report',tag=5,state=1,tx_count=1)],
                cleanup=dict(network_tx_paused=True),attempt=dict(ledger_after_sha256=p.digest(ledger)),
                evidence={'artifacts/raw':p.digest(raw)})
            (root/'research/ack-probe-stage92-observation.json').write_text(json.dumps(previous))
            (root/'artifacts/monitor-channel9-baseline').mkdir()
            baseline_raw=root/'artifacts/monitor-channel9-baseline/monitor-channel9.pcap';baseline_raw.write_bytes(b'baseline')
            baseline=dict(stage=94,frequencies_mhz={'2452':1499},target_beacons_without_bad_fcs_flag=165,
                archer_tx_submissions=0,capture_exit=0,source_sha256=p.digest(baseline_raw))
            (root/'research/monitor-baseline-stage94-observation.json').write_text(json.dumps(baseline))
            helper=root/'build/archer-special-external-probe-universal'
            helper.write_text('#!/usr/bin/env python3\nraise SystemExit(7)\n');helper.chmod(0o700)
            marker=root/'artifacts/37ad-0104-special-channel9-external-probe-attempted'
            with patch.object(p,'ROOT',root),patch.object(p,'LEDGER',ledger):
                with self.assertRaises(ValueError):
                    p.execute(root/'blocked',external_capture=True,capture_guard=lambda:(_ for _ in ()).throw(ValueError('capture stopped')))
                self.assertFalse(marker.exists())
                with self.assertRaises(RuntimeError):p.execute(root/'first',external_capture=True,capture_guard=lambda:None)
                with self.assertRaises(FileExistsError):p.execute(root/'second',external_capture=True,capture_guard=lambda:None)
            self.assertEqual(marker.read_bytes(),b'RECOVERY_RESERVED\n')
            self.assertEqual(old.read_bytes(),b'OLD\n')


    def test_paired_measurement_requires_fresh_baseline_and_restoration(self):
        rows=[dict(operation='probe_ifs_clm_window',index=i,period=62500,unit_us=32,elapsed_ms=2010,tx_units=0 if i==0 else 16) for i in range(2)]
        rows.append(dict(phase='probe_ifs_controls_restored'))
        self.assertEqual(p.validate_measurement(rows)['windows'][1]['tx_units'],16)
        for changed in (rows[:-1],list(reversed(rows))):
            with self.assertRaises(ValueError):p.validate_measurement(changed)
        rows[0]['tx_units']=1
        with self.assertRaises(ValueError):p.validate_measurement(rows)

    def test_control_baseline_rejects_missing_cts_or_stale_profile(self):
        record=dict(stage=91,channel=9,context='CLEAN',remaining_sequences=215,calibration_terminals=8,active_probes_sent=0,scan_live_equals_replay=True,evidence={'raw':'sha'},control_audit=dict(ack_frames=2,cts_frames=1,bad_control_fcs=0))
        p.validate_control_baseline(record)
        for update in (dict(channel=1),dict(context='DIRTY'),dict(calibration_terminals=7),dict(evidence={}),dict(control_audit=dict(ack_frames=2,cts_frames=0,bad_control_fcs=0))):
            with self.assertRaises(ValueError):p.validate_control_baseline({**record,**update})
