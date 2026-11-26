import copy
import unittest
from test_supplicant_result import trace
import wpa2_once as w


def unicast_trace(replies=0, discover_retry=False, request_retry=False):
    rows=trace(replies,discover_retry,request_retry)
    for r in rows:
        if r.get('operation')=='secure_usb_write' and r['phase']>=11:r['phase']+=1
    pos=next(i for i,r in enumerate(rows) if r.get('operation')=='secure_usb_write' and r['phase']==12)
    rows[pos:pos]=[
        dict(operation='secure_usb_write',phase=11,endpoint=9,bytes=140,status=0),
        dict(operation='single_probe_tx_report',tag=15,state=0,tx_count=1),
        dict(operation='secure_unicast_arp_reply',pairwise=True,ccmp_authenticated=True,identity_matched=True,local_replay_rejected=True)]
    pos=next(i for i,r in enumerate(rows) if r.get('operation')=='secure_usb_write' and r['phase']==13)
    rows.insert(pos,dict(operation='secure_data_summary',pairwise_frames=1))
    return rows


class UnicastResultTest(unittest.TestCase):
    def test_optional_protocol_paths_preserve_unicast_and_cleanup(self):
        for replies in range(4):
            for discover in (False,True):
                for request in (False,True):
                    result=w.validate_unicast_result(unicast_trace(replies,discover,request))
                    self.assertTrue(result['pairwise_arp_verified']);self.assertTrue(result['local_replay_rejected'])
                    self.assertFalse(result['os_interface']);self.assertFalse(result['server_release_confirmed'])

    def test_missing_false_duplicated_or_late_proof_fails(self):
        source=unicast_trace()
        for operation in ('secure_unicast_arp_reply','secure_data_summary'):
            changed=[r for r in copy.deepcopy(source) if r.get('operation')!=operation]
            with self.assertRaises(ValueError):w.validate_unicast_result(changed)
        for key in ('pairwise','ccmp_authenticated','identity_matched','local_replay_rejected'):
            changed=copy.deepcopy(source);next(r for r in changed if r.get('operation')=='secure_unicast_arp_reply')[key]=False
            with self.assertRaises(ValueError):w.validate_unicast_result(changed)
        changed=copy.deepcopy(source);reply=next(r for r in changed if r.get('operation')=='secure_unicast_arp_reply');changed.append(reply.copy())
        with self.assertRaises(ValueError):w.validate_unicast_result(changed)
        changed=copy.deepcopy(source);reply=next(r for r in changed if r.get('operation')=='secure_unicast_arp_reply');changed.remove(reply);changed.append(reply)
        with self.assertRaises(ValueError):w.validate_unicast_result(changed)

    def test_duplicate_arp_submission_is_not_hidden_by_phase_mapping(self):
        rows=unicast_trace();position=next(i for i,r in enumerate(rows) if r.get('operation')=='secure_usb_write' and r.get('phase')==11)
        rows.insert(position,rows[position].copy())
        with self.assertRaises(ValueError):w.validate_unicast_result(rows)
