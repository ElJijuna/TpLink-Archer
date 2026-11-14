import copy
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parents[1]/'tools'))
import wpa2_once as w


def trace(replies=0,retry=False):
    rows=[dict(operation='probe_grant_policy',phase='enabled')]
    def write(phase):
        size=94 if phase==0 else 130 if phase==1 else 217 if phase<=5 else 195 if phase==6 else 90 if phase==11 else 440
        rows.append(dict(operation='secure_usb_write',phase=phase,endpoint=5 if phase in (0,1,11) else 9,bytes=size,status=0))
    write(0);rows.append(dict(operation='station_management_reply',kind=10,status=0,aid=0))
    write(1);rows.append(dict(operation='station_management_reply',kind=11,status=0,aid=1))
    rows.append(dict(operation='secure_m1_accepted'));write(2)
    for i in range(replies):
        rows.append(dict(operation='secure_m2_response_retry',ordinal=i+1,same_anonce=True,replay_advanced=True));write(3+i)
    rows.append(dict(operation='secure_m3_authenticated'));write(6);write(7)
    if retry:
        rows.append(dict(operation='secure_discover_retry',ordinal=1,same_xid=True,fresh_pn_and_sequence=True,secs=4));write(8)
    rows.append(dict(operation='secure_dhcp_offer',matched_transaction=True,ccmp_authenticated=True,security_established=True,data_path_established=True,lease_acquired=False))
    write(9);rows.append(dict(operation='secure_dhcp_ack',ccmp_authenticated=True,lease_acknowledged=True,os_interface=False))
    write(10);rows.append(dict(operation='single_probe_tx_report',tag=13,state=0,tx_count=1));rows.append(dict(operation='secure_dhcp_release',tx_done=True,server_release_confirmed=False))
    write(11);rows += [dict(operation='single_probe_tx_report',tag=14,state=0,tx_count=1),dict(result='context_cleanup_acknowledged',network_tx_paused=True),dict(operation='bulk_async_drained',pending=0),dict(operation='probe_grant_policy',phase='restored')]
    return rows


class SupplicantResultTest(unittest.TestCase):
    def test_all_optional_paths(self):
        for count in range(4):
            for retry in (False,True):
                result=w.validate_supplicant_result(trace(count,retry))
                self.assertEqual(result['m1_response_retries'],count);self.assertTrue(result['lease_acquired']);self.assertFalse(result['server_release_confirmed'])

    def test_missing_duplicate_and_unauthorized_responses_fail(self):
        source=trace(3,True)
        for i in range(len(source)):
            changed=copy.deepcopy(source);del changed[i]
            with self.assertRaises(ValueError):w.validate_supplicant_result(changed)
        for key,value in [('same_anonce',False),('replay_advanced',False),('ordinal',4)]:
            changed=copy.deepcopy(source);next(r for r in changed if r.get('operation')=='secure_m2_response_retry')[key]=value
            with self.assertRaises(ValueError):w.validate_supplicant_result(changed)
        changed=copy.deepcopy(source);changed.insert(6,changed[5].copy())
        with self.assertRaises(ValueError):w.validate_supplicant_result(changed)

    def test_m3_must_precede_m4_after_last_m2_response(self):
        changed=trace(2);m3=next(r for r in changed if r.get('operation')=='secure_m3_authenticated');changed.remove(m3);changed.append(m3)
        with self.assertRaises(ValueError):w.validate_supplicant_result(changed)
