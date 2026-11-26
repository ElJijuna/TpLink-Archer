import copy
import unittest
from unittest.mock import patch
from test_unicast_result import unicast_trace
import wpa2_once as w


def echo_trace():
    rows=unicast_trace()
    for r in rows:
        if r.get('operation')=='secure_usb_write' and r['phase']>=12:r['phase']+=1
        if r.get('operation')=='secure_data_summary':r['pairwise_frames']=2
    pos=next(i for i,r in enumerate(rows) if r.get('operation')=='secure_usb_write' and r['phase']==13)
    rows[pos:pos]=[dict(operation='secure_usb_write',phase=12,endpoint=9,bytes=156,status=0),dict(operation='single_probe_tx_report',tag=9,state=0,tx_count=1),dict(operation='secure_unicast_echo_reply',pairwise=True,ccmp_authenticated=True,identity_matched=True,echo_cookie_matched=True,local_replay_rejected=True)]
    return rows


class EchoResultTest(unittest.TestCase):
    def test_complete_ip_exchange_retains_cleanup_limits(self):
        result=w.validate_echo_result(echo_trace())
        self.assertTrue(result['gateway_echo_verified']);self.assertTrue(result['pairwise_arp_verified']);self.assertFalse(result['os_interface']);self.assertFalse(result['server_release_confirmed'])

    def test_duplicate_missing_or_false_echo_proof_fails(self):
        for field in ('pairwise','ccmp_authenticated','identity_matched','echo_cookie_matched','local_replay_rejected'):
            rows=echo_trace();next(r for r in rows if r.get('operation')=='secure_unicast_echo_reply')[field]=False
            with self.assertRaises(ValueError):w.validate_echo_result(rows)
        rows=echo_trace();pos=next(i for i,r in enumerate(rows) if r.get('operation')=='secure_usb_write' and r['phase']==12);rows.insert(pos,rows[pos].copy())
        with self.assertRaises(ValueError):w.validate_echo_result(rows)
        rows=[r for r in echo_trace() if r.get('operation')!='secure_unicast_echo_reply']
        with self.assertRaises(ValueError):w.validate_echo_result(rows)

    @patch.object(w,'digest',return_value='hash')
    def test_continuation_requires_completed_arp_and_unchanged_clean_epoch(self,_):
        history=dict(stage=180,status='completed_unicast_arp_and_context_verified',context='CLEAN',epoch_transition='CLEAN',remaining_sequences=206,ledger_sha256='hash',epoch_id='epoch',validation=dict(pairwise_arp_verified=True,local_replay_rejected=True,release_submitted=True),context_check=dict(ack_steps=[0,1,2,3,4,6,7,8],network_tx_paused=True),postflight=dict(registers=210),evidence={'capture':'hash'})
        ledger=b'ARCHSEQ\x01'+bytes([1])*50+bytes(206)
        w.validate_clean_unicast_history(history,ledger,'epoch')
        for field,value in [('context','DIRTY'),('epoch_id','old'),('remaining_sequences',215),('evidence',{})]:
            bad=copy.deepcopy(history);bad[field]=value
            with self.assertRaises(ValueError):w.validate_clean_unicast_history(bad,ledger,'epoch')
        with self.assertRaises(ValueError):w.validate_clean_unicast_history(history,b'ARCHSEQ\x01'+bytes([1])*41+bytes(215),'epoch')
