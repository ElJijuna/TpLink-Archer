import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).parents[1]


class OfflineCaptureAuditTest(unittest.TestCase):
    def test_recorded_handshake_and_encrypted_offer_with_public_dummy_key(self):
        with tempfile.TemporaryDirectory() as tmp:
            capture=Path(tmp)/'capture'
            subprocess.run([str(ROOT/'build/wpa2-audit-fixture'),str(capture)],cwd=ROOT,check=True)
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
                else:self.assertEqual(result.stdout,'')
