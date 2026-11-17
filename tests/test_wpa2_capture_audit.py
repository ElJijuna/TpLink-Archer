import json
import os
from pathlib import Path
import subprocess
import shutil
import tempfile
import unittest
ROOT=Path(__file__).parents[1]


class OfflineCaptureAuditTest(unittest.TestCase):
    def test_recorded_handshake_and_encrypted_offer_with_public_dummy_key(self):
        self.audit_fixture(False)

    def test_broadcast_discover_is_auditable_without_becoming_an_offer(self):
        self.audit_fixture(True)

    def test_reference_authenticated_before_attempt_m2_comparison(self):
        with tempfile.TemporaryDirectory() as tmp:
            ref=Path(tmp)/'reference';attempt=Path(tmp)/'attempt'
            subprocess.run([str(ROOT/'build/wpa2-audit-fixture'),str(ref)],cwd=ROOT,check=True);shutil.copytree(ref,attempt)
            original=(attempt/'secure-tx-2.bin').read_bytes()
            for mutated,credential,success in [(False,b'password',True),(True,b'password',True),(False,b'wrong-password',False)]:
                packet=bytearray(original)
                if mutated:packet[177]^=1
                (attempt/'secure-tx-2.bin').write_bytes(packet)
                fd,writer=os.pipe();os.write(writer,credential);os.close(writer)
                try:
                    result=subprocess.run([str(ROOT/'build/archer-wpa2-capture-audit'),str(ref),str(attempt)],cwd=ROOT,env=dict(os.environ,ARCHER_CREDENTIAL_FD=str(fd)),pass_fds=(fd,),capture_output=True,text=True,check=False)
                finally:os.close(fd)
                self.assertEqual(result.returncode==0,success);self.assertNotIn(credential.decode(),result.stdout+result.stderr)
                if success:
                    rows=[json.loads(x) for x in result.stdout.splitlines()];comparisons=[r for r in rows if r.get('operation')=='offline_credential_comparison']
                    self.assertEqual(len(comparisons),1);self.assertTrue(comparisons[0]['reference_m3_authenticated']);self.assertEqual(comparisons[0]['attempt_m2_reproduced'],not mutated)
                    self.assertEqual(comparisons[0]['network_submissions'],0)
                else:self.assertEqual(result.stdout,'')

    def test_explicit_phase_layout_selects_new_m4_and_discover_slots(self):
        self.audit_fixture(False,True)

    def test_recorded_request_ack_release_and_ciphertext_corruption(self):
        with tempfile.TemporaryDirectory() as tmp:
            capture=Path(tmp)/'capture'
            subprocess.run([str(ROOT/'build/wpa2-lease-audit-fixture'),str(capture),'lease'],cwd=ROOT,check=True)
            originals={n:(capture/n).read_bytes() for n in ('secure-tx-9.bin','secure-tx-10.bin')}
            for corrupt in (None,'secure-tx-9.bin','secure-tx-10.bin'):
                for name,data in originals.items():(capture/name).write_bytes(data)
                if corrupt:
                    data=bytearray(originals[corrupt]);data[110]^=1;(capture/corrupt).write_bytes(data)
                fd,writer=os.pipe();os.write(writer,b'password');os.close(writer)
                try:result=subprocess.run([str(ROOT/'build/archer-wpa2-capture-audit'),str(capture)],cwd=ROOT,env=dict(os.environ,ARCHER_CREDENTIAL_FD=str(fd)),pass_fds=(fd,),capture_output=True,text=True,check=False)
                finally:os.close(fd)
                self.assertEqual(result.returncode==0,corrupt is None);self.assertNotIn('password',result.stdout+result.stderr);self.assertNotIn('192.168.1.55',result.stdout+result.stderr)
                if corrupt is None:
                    summary=json.loads(result.stdout.splitlines()[-1]);self.assertEqual(summary['matched_acks'],1)
                    self.assertTrue(summary['recorded_request_verified']);self.assertTrue(summary['recorded_release_verified']);self.assertFalse(summary['server_release_confirmed'])

    def audit_fixture(self,broadcast,layout=False):
        with tempfile.TemporaryDirectory() as tmp:
            capture=Path(tmp)/'capture'
            subprocess.run([str(ROOT/'build/wpa2-audit-fixture'),str(capture)]+(['broadcast'] if broadcast else []),cwd=ROOT,check=True)
            if layout:
                (capture/'secure-tx-3.bin').rename(capture/'secure-tx-6.bin');(capture/'secure-tx-4.bin').rename(capture/'secure-tx-7.bin')
                (capture/'secure-layout.txt').write_text('wpa2-phase-layout-v1\n2 6 7\n')
            for credential,success in [(b'password',True),(b'wrong-password',False)]:
                fd,writer=os.pipe();os.write(writer,credential);os.close(writer)
                try:
                    result=subprocess.run([str(ROOT/'build/archer-wpa2-capture-audit'),str(capture)],cwd=ROOT,env=dict(os.environ,ARCHER_CREDENTIAL_FD=str(fd)),pass_fds=(fd,),capture_output=True,text=True,check=False)
                finally:os.close(fd)
                self.assertEqual(result.returncode==0,success)
                self.assertNotIn(credential.decode(),result.stdout+result.stderr)
                if success:
                    rows=[json.loads(x) for x in result.stdout.splitlines()];summary=rows[-1]
                    self.assertEqual(summary['matched_offers'],1);self.assertEqual(summary['pairwise_frames'],1);self.assertEqual(summary['reflected_discover_frames'],1)
                    self.assertTrue(summary['m3_authenticated']);self.assertTrue(summary['recorded_discover_verified'])
                    self.assertEqual(summary['network_submissions'],0)
                    arp=[r for r in rows if r.get('ether_type')==2054]
                    self.assertEqual(len(arp),1);self.assertEqual(arp[0]['arp_operation'],1)
                    self.assertTrue(arp[0]['arp_sender_ip_zero']);self.assertFalse(arp[0]['arp_target_ip_zero'])
                    self.assertTrue(arp[0]['arp_sender_is_ap']);self.assertTrue(arp[0]['arp_target_is_station'])
                    self.assertNotIn('192.0.2.10',result.stdout)
                    self.assertEqual(summary['authenticated_frames'],3);self.assertEqual(summary['rejected_frames'],0)
                else:self.assertEqual(result.stdout,'')
