"""One owned Especial WPA2/CCMP experiment; no credential in files or arguments."""
import fcntl
import getpass
import json
import os
from pathlib import Path
import shutil
import subprocess
from contextlib import contextmanager
from passive_workflow import ROOT, LEDGER, save, digest

MARKER=ROOT/'artifacts/37ad-0104-special-channel9-wpa2-attempted'
HISTORY=ROOT/'research/eapol-rx-stage104-observation.json'


def validate_history(history, ledger):
    if (history.get('stage')!=104 or history.get('status')!='association_and_native_eapol_m1_verified'
        or history.get('context')!='CLEAN' or history.get('remaining_sequences')!=152
        or history.get('ledger_sha256')!=digest(LEDGER)
        or ledger!=b'ARCHSEQ\x01'+bytes([1])*104+bytes(152)
        or history.get('result',{}).get('association_verified') is not True
        or history.get('result',{}).get('security_established') is not False
        or history.get('native_audit',{}).get('target_m1_frames')!=1
        or history.get('grant_restored') is not True):
        raise ValueError('Verified stage104 M1 and unchanged sequence history required')
    if not history.get('evidence'):raise ValueError('Retained physical evidence required')


def admission():
    readiness=json.loads((ROOT/'research/wpa2-stage105-observation.json').read_text())
    if readiness.get('status')!='prepared_not_executed' or readiness.get('stage')!=105:raise ValueError('Validated stage105 build required')
    for name,sha in readiness['build_evidence'].items():
        path=(ROOT/name).resolve()
        if not path.is_relative_to(ROOT) or digest(path)!=sha:raise ValueError('Stage105 tested build/source changed')
    history=json.loads(HISTORY.read_text());validate_history(history,LEDGER.read_bytes())
    for name,sha in history['evidence'].items():
        path=(ROOT/name).resolve()
        if not path.is_relative_to(ROOT/'artifacts') or digest(path)!=sha:
            raise ValueError('Stage104 retained evidence changed')
    for path in (ROOT/'artifacts/37ad-0104-context-reservation',Path(str(LEDGER)+'.epoch-transition')):
        if path.read_bytes()!=b'CLEAN\n':raise ValueError('Context/epoch not CLEAN')
    if (ROOT/'artifacts/37ad-0104-special-channel9-eapol-rx-attempted').read_bytes()!=b'RECOVERY_RESERVED\n':
        raise ValueError('Prior one-shot marker must remain intact')
    if MARKER.exists():raise ValueError('WPA2 attempt already reserved; no retry')


@contextmanager
def credential_pipe():
    if not os.isatty(0):raise ValueError('Enter the passphrase locally in an interactive Terminal')
    secret=getpass.getpass('Especial WPA2 passphrase (local, hidden): ')
    encoded=bytearray(secret,'utf-8');del secret
    read_fd=write_fd=None
    try:
        if not 8<=len(encoded)<=63 or 0 in encoded:raise ValueError('Passphrase must be8–63 UTF-8 bytes without NUL')
        read_fd,write_fd=os.pipe()
        if os.write(write_fd,encoded)!=len(encoded):raise OSError('Incomplete private credential transfer')
        os.close(write_fd);write_fd=None
        encoded[:]=bytes(len(encoded))
        yield read_fd
    finally:
        encoded[:]=bytes(len(encoded))
        if write_fd is not None:os.close(write_fd)
        if read_fd is not None:os.close(read_fd)


def validate_result(rows):
    writes=[r for r in rows if r.get('operation')=='secure_usb_write']
    if [r.get('phase') for r in writes]!=list(range(6)) or any(r.get('status')!=0 for r in writes):
        raise ValueError('Six unique successful USB submissions required')
    if [r.get('bytes') for r in writes]!=[94,130,217,195,440,90]:raise ValueError('Secure packet lengths mismatch')
    if [r.get('endpoint') for r in writes]!=[5,5,9,9,9,5]:raise ValueError('Secure endpoint order mismatch')
    for operation in ('secure_m1_accepted','secure_m3_authenticated','secure_dhcp_offer'):
        if sum(r.get('operation')==operation for r in rows)!=1:raise ValueError('Missing unique secure phase evidence')
    offer=next(r for r in rows if r.get('operation')=='secure_dhcp_offer')
    if any(offer.get(k) is not True for k in ('matched_transaction','ccmp_authenticated','security_established','data_path_established')) or offer.get('lease_acquired') is not False:
        raise ValueError('Matched authenticated DHCP OFFER required; no lease claim')
    replies=[r for r in rows if r.get('operation')=='station_management_reply']
    if [r.get('kind') for r in replies]!=[10,11] or any(r.get('status')!=0 for r in replies):raise ValueError('AP authentication and association required')
    if not 1<=replies[1].get('aid',0)<=2007:raise ValueError('Valid association AID required')
    reports=[r for r in rows if r.get('operation')=='single_probe_tx_report']
    if not reports or any((r.get('tag'),r.get('state'),r.get('tx_count'))!=(0,0,1) for r in reports):raise ValueError('Fresh tag0 cleanup TX_DONE required')
    if not any(r.get('result')=='context_cleanup_acknowledged' and r.get('network_tx_paused') is True for r in rows):raise ValueError('Context cleanup required')
    if not any(r.get('operation')=='bulk_async_drained' and r.get('pending')==0 for r in rows):raise ValueError('Async drain required')
    # Native window helpers enforce restoration; require both logged grant boundaries.
    if [r.get('phase') for r in rows if r.get('operation')=='probe_grant_policy']!=['enabled','restored']:raise ValueError('Grant restoration evidence required')
    return dict(security_established=True,data_path_established=True,lease_acquired=False,os_interface=False)


def execute(output,capture_guard,credential_fd):
    output=output.resolve();output.mkdir();fds=[]
    report=dict(status='preflight',attempt='wpa2_once',wifi_sequences=list(range(16,22)),cleanup_report_tag=0,automatic_retry=False)
    try:
        owner=os.open(str(LEDGER)+'.workflow',os.O_RDWR|os.O_CREAT|os.O_NOFOLLOW,0o600);fds.append(owner);fcntl.flock(owner,fcntl.LOCK_EX|fcntl.LOCK_NB)
        admission()
        helper=output/'archer-special-wpa2';shutil.copy2(ROOT/'build/archer-special-wpa2-universal',helper)
        report.update(history_sha256=digest(HISTORY),helper_sha256=digest(helper),ledger_before_sha256=digest(LEDGER));save(output/'attempt.json',report)
        capture_guard()
        permit=os.open(MARKER,os.O_RDWR|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600);fds.append(permit);fcntl.flock(permit,fcntl.LOCK_EX|fcntl.LOCK_NB)
        token=b'RECOVERY_RESERVED\n'
        if os.write(permit,token)!=len(token):raise OSError('Incomplete attempt reservation')
        os.fsync(permit)
        parent=os.open(MARKER.parent,os.O_RDONLY)
        try:os.fsync(parent)
        finally:os.close(parent)
        report['status']='started';save(output/'attempt.json',report)
        env=dict(os.environ,ARCHER_WORKFLOW_FD=str(owner),ARCHER_PROBE_RECOVERY_FD=str(permit),ARCHER_CREDENTIAL_FD=str(credential_fd))
        with (output/'probe.jsonl').open('w') as out,(output/'probe.stderr').open('w') as err:
            result=subprocess.run([str(helper),'--single-special-channel9-wpa2',str(output/'capture')],cwd=ROOT,env=env,pass_fds=(owner,permit,credential_fd),stdout=out,stderr=err,check=False)
        report.update(exit_code=result.returncode,stdout_sha256=digest(output/'probe.jsonl'),stderr_sha256=digest(output/'probe.stderr'),ledger_after_sha256=digest(LEDGER))
        if result.returncode:raise RuntimeError('WPA2 attempt stopped; reservation retained, no retry')
        report['validation']=validate_result([json.loads(line) for line in (output/'probe.jsonl').read_text().splitlines()])
        report['status']='completed';save(output/'attempt.json',report)
    except BaseException as error:
        report.update(status='stopped',error=str(error));save(output/'attempt.json',report);raise
    finally:
        for fd in reversed(fds):os.close(fd)
