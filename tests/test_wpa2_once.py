import copy
import json
import os
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).parents[1]/'tools'))
import wpa2_once as w


def rows():
    result=[dict(operation='secure_usb_write',phase=i,endpoint=e,bytes=n,status=0) for i,e,n in zip(range(6),[5,5,9,9,9,5],[94,130,217,195,440,90])]
    result += [dict(operation='secure_m1_accepted'),dict(operation='secure_m3_authenticated'),dict(operation='secure_dhcp_offer',matched_transaction=True,ccmp_authenticated=True,security_established=True,data_path_established=True,lease_acquired=False)]
    result += [dict(operation='station_management_reply',kind=k,status=0,aid=2 if k==11 else 0) for k in [10,11]]
    result += [dict(operation='single_probe_tx_report',tag=0,state=0,tx_count=1),dict(result='context_cleanup_acknowledged',network_tx_paused=True),dict(operation='bulk_async_drained',pending=0)]
    result += [dict(operation='probe_grant_policy',phase=p) for p in ['enabled','restored']]
    return result


class Wpa2OnceTest(unittest.TestCase):
    def test_result_requires_every_phase_and_authenticated_offer(self):
        expected=w.validate_result(rows());self.assertTrue(expected['data_path_established']);self.assertFalse(expected['lease_acquired'])
        for i in range(len(rows())):
            changed=rows();del changed[i]
            with self.assertRaises(ValueError):w.validate_result(changed)
        for key in ['matched_transaction','ccmp_authenticated','security_established','data_path_established','lease_acquired']:
            changed=rows();offer=next(r for r in changed if r.get('operation')=='secure_dhcp_offer');offer[key]=not offer[key]
            with self.assertRaises(ValueError):w.validate_result(changed)
        changed=rows();changed[2]['endpoint']=5
        with self.assertRaises(ValueError):w.validate_result(changed)
        changed=rows();changed.insert(3,changed[2].copy())
        with self.assertRaises(ValueError):w.validate_result(changed)

    def test_private_pipe_local_dummy_and_eof(self):
        with patch.object(w.os,'isatty',return_value=True),patch.object(w.getpass,'getpass',return_value='dummy-password'):
            with w.credential_pipe() as fd:
                self.assertEqual(os.read(fd,64),b'dummy-password');self.assertEqual(os.read(fd,1),b'')
            with self.assertRaises(OSError):os.fstat(fd)
        for value in ['short','a'*64,'password\0']:
            with patch.object(w.os,'isatty',return_value=True),patch.object(w.getpass,'getpass',return_value=value):
                with self.assertRaises(ValueError):
                    with w.credential_pipe():self.fail('Invalid credential accepted')
        with patch.object(w.os,'isatty',return_value=False),patch.object(w.getpass,'getpass') as prompt:
            with self.assertRaises(ValueError):
                with w.credential_pipe():self.fail('TTY required')
            prompt.assert_not_called()

    def test_history_pins_native_m1_and_sequence_domain(self):
        history=json.loads(w.HISTORY.read_text());ledger=b'ARCHSEQ\x01'+bytes([1])*104+bytes(152)
        with patch.object(w,'digest',return_value=history['ledger_sha256']):
            w.validate_history(history,ledger)
            for key,value in [('remaining_sequences',151),('context','DIRTY'),('grant_restored',False)]:
                changed=copy.deepcopy(history);changed[key]=value
                with self.assertRaises(ValueError):w.validate_history(changed,ledger)
            with self.assertRaises(ValueError):w.validate_history(history,ledger[:-1]+b'\x01')
