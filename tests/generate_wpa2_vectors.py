"""Deterministic public test keys only; Python HMAC/PBKDF2 + OpenSSL3 AES wrap.
Never accepts a real credential or performs USB/network access.
"""
from pathlib import Path
import hashlib
import hmac
import subprocess
import tempfile
import os

ROOT=Path(__file__).resolve().parent/'fixtures'
frame=(ROOT/'special-eapol-m1-stage104.bin').read_bytes()
m1=frame[32:-4];assert m1[:4]==bytes.fromhex('0203005f')
anonce=m1[17:49];snonce=bytes(range(1,33));replay=m1[9:17]
pmk=hashlib.pbkdf2_hmac('sha1',b'password',b'Especial',4096,32)
sta=bytes.fromhex('0200008922a1');ap=bytes.fromhex('50926a110666')
data=min(sta,ap)+max(sta,ap)+min(anonce,snonce)+max(anonce,snonce)
ptk=b''.join(hmac.new(pmk,b'Pairwise key expansion\x00'+data+bytes([i]),'sha1').digest() for i in range(3))[:48]
rsn=bytes.fromhex('30140100000fac040100000fac040100000fac020000')
def key(info,replay,nonce,data=b'',length=0):
    body=bytearray(95+len(data));body[0]=2;body[1:3]=info.to_bytes(2,'big');body[3:5]=length.to_bytes(2,'big')
    body[5:13]=replay;body[13:45]=nonce;body[93:95]=len(data).to_bytes(2,'big');body[95:]=data
    result=bytearray(bytes([2,3])+len(body).to_bytes(2,'big')+body)
    result[81:97]=hmac.new(ptk[:16],result,'sha1').digest()[:16];return result

def wrap(raw):
    assert len(raw)%8==0
    with tempfile.NamedTemporaryFile() as source:
        source.write(raw);source.flush()
        command=os.environ.get('ARCHER_TEST_OPENSSL','/opt/homebrew/opt/openssl@3/bin/openssl')
        return subprocess.run([command,'enc','-id-aes128-wrap','-K',ptk[16:32].hex(),'-iv','a6'*8,'-nosalt','-in',source.name],capture_output=True,check=True).stdout

replay3=(int.from_bytes(replay,'big')+1).to_bytes(8,'big')
ap_rsn=rsn[:-2]+bytes.fromhex('0c00')
gtk=bytes.fromhex('dd16000fac010100')+bytes(range(0xa0,0xb0))
raw=ap_rsn+gtk+bytes.fromhex('dd00');assert len(raw)==48
vectors={'wpa2-test-ptk.bin':ptk,'wpa2-test-m2.bin':key(0x010a,replay,snonce,rsn),
         'wpa2-test-m3.bin':key(0x13ca,replay3,anonce,wrap(raw),16),
         'wpa2-test-m4.bin':key(0x030a,replay3,bytes(32))}
bad=bytearray(raw);bad[len(ap_rsn)+7]=1
vectors['wpa2-test-m3-invalid-kde.bin']=key(0x13ca,replay3,anonce,wrap(bad),16)
for name,value in vectors.items():(ROOT/name).write_bytes(value)
print('Generated five public WPA2 test vectors; no real credential or hardware access')
