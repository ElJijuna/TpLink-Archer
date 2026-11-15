"""One owned Especial WPA2/CCMP experiment; no credential in files or arguments."""
import fcntl
import copy
import getpass
import hmac
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


RECOVERED_HISTORY=ROOT/'research/recovery-stage137-observation.json'
RECOVERED_MARKER=ROOT/'artifacts/37ad-0104-special-channel9-wpa2-recovered-stage138-attempted'
LEASE_HISTORY=ROOT/'research/recovery-stage147-observation.json'
LEASE_MARKER=ROOT/'artifacts/37ad-0104-special-channel9-dhcp-lease-stage148-attempted'
SUPPLICANT_HISTORY=ROOT/'research/recovery-stage161-observation.json'
SUPPLICANT_MARKER=ROOT/'artifacts/37ad-0104-special-channel9-eapol-retry-stage162-attempted'

def validate_recovered_history(history,ledger,epoch_id,expected_stage=137):
    if (history.get('stage')!=expected_stage or history.get('status')!='cold_recovery_and_context_verified'
        or history.get('context')!='CLEAN' or history.get('epoch_transition')!='CLEAN'
        or history.get('remaining_sequences')!=215 or history.get('ledger_sha256')!=digest(LEDGER)
        or ledger!=b'ARCHSEQ\x01'+bytes([1])*41+bytes(215)
        or history.get('epoch_boundary',{}).get('status')!='committed'
        or not epoch_id or history.get('epoch_boundary',{}).get('epoch_id')!=epoch_id
        or history.get('context_check',{}).get('ack_steps')!=[0,1,2,3,4,6,7,8]
        or history.get('context_check',{}).get('network_tx_paused') is not True
        or history.get('postflight',{}).get('registers')!=210):
        raise ValueError('Verified cold epoch and passive context check required')
    if not history.get('evidence'):raise ValueError('Retained recovered evidence required')


def admission(recovered=False,lease=False,supplicant=False):
    if supplicant:lease=True
    if lease:recovered=True
    readiness=json.loads((ROOT/('research/supplicant-stage162-observation.json' if supplicant else 'research/dhcp-stage148-observation.json' if lease else 'research/wpa2-stage138-observation.json' if recovered else 'research/wpa2-stage105-observation.json')).read_text())
    if readiness.get('status')!='prepared_not_executed' or readiness.get('stage')!=(162 if supplicant else 148 if lease else 138 if recovered else 105):raise ValueError('WPA2 state/build not prepared: '+str(readiness.get('status')))
    for name,sha in readiness['build_evidence'].items():
        path=(ROOT/name).resolve()
        if not path.is_relative_to(ROOT) or digest(path)!=sha:raise ValueError('Tested WPA2 build/source changed')
    history=json.loads((SUPPLICANT_HISTORY if supplicant else LEASE_HISTORY if lease else RECOVERED_HISTORY if recovered else HISTORY).read_text())
    if recovered:validate_recovered_history(history,LEDGER.read_bytes(),Path(str(LEDGER)+'.epoch-id').read_text().strip(),161 if supplicant else 147 if lease else 137)
    else:validate_history(history,LEDGER.read_bytes())
    for name,sha in history['evidence'].items():
        path=(ROOT/name).resolve()
        if not path.is_relative_to(ROOT/'artifacts') or digest(path)!=sha:
            raise ValueError('Retained physical evidence changed')
    for path in (ROOT/'artifacts/37ad-0104-context-reservation',Path(str(LEDGER)+'.epoch-transition')):
        if path.read_bytes()!=b'CLEAN\n':raise ValueError('Context/epoch not CLEAN')
    if (ROOT/'artifacts/37ad-0104-special-channel9-eapol-rx-attempted').read_bytes()!=b'RECOVERY_RESERVED\n':
        raise ValueError('Prior one-shot marker must remain intact')
    if recovered and MARKER.read_bytes()!=b'RECOVERY_RESERVED\n':raise ValueError('Original WPA2 reservation must remain intact')
    if recovered and (ROOT/'artifacts/37ad-0104-special-channel9-wpa2-recovered-stage116-attempted').read_bytes()!=b'RECOVERY_RESERVED\n':raise ValueError('Stage116 reservation must remain intact')
    if recovered and (ROOT/'artifacts/37ad-0104-special-channel9-wpa2-recovered-stage122-attempted').read_bytes()!=b'RECOVERY_RESERVED\n':raise ValueError('Stage122 reservation must remain intact')
    if recovered and (ROOT/'artifacts/37ad-0104-special-channel9-wpa2-recovered-stage133-attempted').read_bytes()!=b'RECOVERY_RESERVED\n':raise ValueError('Stage133 reservation must remain intact')
    if lease and RECOVERED_MARKER.read_bytes()!=b'RECOVERY_RESERVED\n':raise ValueError('Stage138 reservation must remain intact')
    if lease and (ROOT/'artifacts/37ad-0104-special-channel9-dhcp-lease-stage141-attempted').read_bytes()!=b'RECOVERY_RESERVED\n':raise ValueError('Stage141 reservation must remain intact')
    if supplicant and LEASE_MARKER.read_bytes()!=b'RECOVERY_RESERVED\n':raise ValueError('Stage148 reservation must remain intact')
    if (SUPPLICANT_MARKER if supplicant else LEASE_MARKER if lease else RECOVERED_MARKER if recovered else MARKER).exists():raise ValueError('WPA2 attempt already reserved; no retry')


@contextmanager
def credential_pipe(confirm=False):
    if not os.isatty(0):raise ValueError('Enter the passphrase locally in an interactive Terminal')
    secret=getpass.getpass('Especial WPA2 passphrase (local, hidden): ')
    encoded=bytearray(secret,'utf-8');del secret
    read_fd=write_fd=None
    try:
        if not 8<=len(encoded)<=63 or 0 in encoded:raise ValueError('Passphrase must be8–63 UTF-8 bytes without NUL')
        if confirm:
            confirmation=bytearray(getpass.getpass('Repeat Especial passphrase (local, hidden): '),'utf-8')
            try:
                if not hmac.compare_digest(encoded,confirmation):raise ValueError('Passphrases differ; no credential transfer or radio submission')
            finally:confirmation[:]=bytes(len(confirmation))
        read_fd,write_fd=os.pipe()
        if os.write(write_fd,encoded)!=len(encoded):raise OSError('Incomplete private credential transfer')
        os.close(write_fd);write_fd=None
        encoded[:]=bytes(len(encoded))
        yield read_fd
    finally:
        encoded[:]=bytes(len(encoded))
        if write_fd is not None:os.close(write_fd)
        if read_fd is not None:os.close(read_fd)


def validate_result(rows,recovered=False):
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
    if not reports or any((r.get('tag'),r.get('state'),r.get('tx_count'))!=((6 if recovered else 0),0,1) for r in reports):raise ValueError('Fresh epoch cleanup TX_DONE required')
    if not any(r.get('result')=='context_cleanup_acknowledged' and r.get('network_tx_paused') is True for r in rows):raise ValueError('Context cleanup required')
    if not any(r.get('operation')=='bulk_async_drained' and r.get('pending')==0 for r in rows):raise ValueError('Async drain required')
    # Native window helpers enforce restoration; require both logged grant boundaries.
    if [r.get('phase') for r in rows if r.get('operation')=='probe_grant_policy']!=['enabled','restored']:raise ValueError('Grant restoration evidence required')
    return dict(security_established=True,data_path_established=True,lease_acquired=False,os_interface=False)


def validate_lease_result(rows):
    writes=[r for r in rows if r.get('operation')=='secure_usb_write']
    phases=[r.get('phase') for r in writes]
    if phases not in ([0,1,2,3,4,6,7,8],list(range(9))) or any(r.get('status')!=0 for r in writes):raise ValueError('Lease phases with at most one unique DISCOVER retry required')
    if [r.get('bytes') for r in writes]!=[94 if p==0 else 130 if p==1 else 217 if p==2 else 195 if p==3 else 90 if p==8 else 440 for p in phases] or [r.get('endpoint') for r in writes]!=[5 if p in (0,1,8) else 9 for p in phases]:raise ValueError('Lease packet lengths/endpoints mismatch')
    ack=[r for r in rows if r.get('operation')=='secure_dhcp_ack']
    release=[r for r in rows if r.get('operation')=='secure_dhcp_release']
    if len(ack)!=1 or ack[0].get('ccmp_authenticated') is not True or ack[0].get('lease_acknowledged') is not True or ack[0].get('os_interface') is not False:raise ValueError('Authenticated matching ACK required')
    if len(release)!=1 or release[0].get('tx_done') is not True or release[0].get('server_release_confirmed') is not False:raise ValueError('RELEASE submission evidence required without server claim')
    reports=[r for r in rows if r.get('operation')=='single_probe_tx_report']
    if {r.get('tag') for r in reports}!={9,10} or any(r.get('state')!=0 or r.get('tx_count')!=1 for r in reports):raise ValueError('Distinct RELEASE/deauth TX_DONE reports required')
    pos=lambda op:next((i for i,r in enumerate(rows) if r.get('operation')==op),-1)
    write_pos={r['phase']:i for i,r in enumerate(rows) if r.get('operation')=='secure_usb_write'}
    report_pos={tag:next(i for i,r in enumerate(rows) if r.get('operation')=='single_probe_tx_report' and r.get('tag')==tag) for tag in (9,10)}
    if not (pos('secure_dhcp_offer')<write_pos[6]<pos('secure_dhcp_ack')<write_pos[7]<report_pos[9]<=pos('secure_dhcp_release')<write_pos[8]<report_pos[10]):raise ValueError('Lease response/report chronology mismatch')
    retries=[r for r in rows if r.get('operation')=='secure_discover_retry']
    if len(retries)!=(1 if 5 in phases else 0) or any(r.get('ordinal')!=1 or r.get('same_xid') is not True or r.get('fresh_pn_and_sequence') is not True or not 3<=r.get('secs',-1)<=30 for r in retries):raise ValueError('Bounded fresh DISCOVER retry evidence required')
    if retries and not write_pos[4]<pos('secure_discover_retry')<write_pos[5]<pos('secure_dhcp_offer'):raise ValueError('DISCOVER retry chronology mismatch')
    canonical=[]
    for row in copy.deepcopy(rows):
        if row.get('operation')=='secure_usb_write':
            if row['phase'] in (5,6,7):continue
            if row['phase']==8:row['phase']=5
        if row.get('operation')=='single_probe_tx_report':
            if row['tag']==9:continue
            row['tag']=6
        canonical.append(row)
    result=validate_result(canonical,True)
    result.update(lease_acquired=True,release_submitted=True,server_release_confirmed=False)
    return result


def validate_supplicant_result(rows):
    writes=[r for r in rows if r.get('operation')=='secure_usb_write'];phases=[r.get('phase') for r in writes]
    retries=[r for r in rows if r.get('operation')=='secure_m2_response_retry'];count=len(retries)
    expected=[0,1,2]+list(range(3,3+count))+[6,7]+([8] if 8 in phases else [])+[9,10,11]
    if count>3 or phases!=expected or any(r.get('status')!=0 for r in writes):raise ValueError('Bounded supplicant phase sequence required')
    if [r.get('bytes') for r in writes]!=[94 if p==0 else 130 if p==1 else 217 if p<=5 else 195 if p==6 else 90 if p==11 else 440 for p in phases] or [r.get('endpoint') for r in writes]!=[5 if p in (0,1,11) else 9 for p in phases]:raise ValueError('Supplicant lengths/endpoints mismatch')
    if [r.get('ordinal') for r in retries]!=list(range(1,count+1)) or any(r.get('same_anonce') is not True or r.get('replay_advanced') is not True for r in retries):raise ValueError('Bounded matching M1 response metadata required')
    write_pos={r['phase']:i for i,r in enumerate(rows) if r.get('operation')=='secure_usb_write'}
    for ordinal,r in enumerate(retries):
        pos=rows.index(r)
        if not write_pos[2+ordinal]<pos<write_pos[3+ordinal]:raise ValueError('M1 response chronology mismatch')
    m3=[i for i,r in enumerate(rows) if r.get('operation')=='secure_m3_authenticated']
    if len(m3)!=1 or not write_pos[2+count]<m3[0]<write_pos[6]:raise ValueError('M3 must authenticate before M4')
    reports=[r for r in rows if r.get('operation')=='single_probe_tx_report']
    if {r.get('tag') for r in reports}!={13,14}:raise ValueError('Distinct supplicant RELEASE/deauth reports required')
    canonical=[]
    for row in copy.deepcopy(rows):
        if row.get('operation')=='secure_usb_write':
            if row['phase'] in (3,4,5):continue
            if row['phase']>=6:row['phase']-=3
        if row.get('operation')=='single_probe_tx_report':row['tag']-=4
        canonical.append(row)
    result=validate_lease_result(canonical);result['m1_response_retries']=count;return result


def execute(output,capture_guard,credential_fd,recovered=False,lease=False,supplicant=False):
    if supplicant:lease=True
    if lease:recovered=True
    output=output.resolve();output.mkdir();fds=[]
    report=dict(status='preflight',attempt='eapol_retry' if supplicant else 'dhcp_lease' if lease else 'wpa2_recovered' if recovered else 'wpa2_once',wifi_sequences=list(range(100,112) if supplicant else range(67,76) if lease else range(52,58) if recovered else range(16,22)),cleanup_report_tag=14 if supplicant else 10 if lease else 6 if recovered else 0,automatic_retry=False,protocol_discover_retransmissions_max=1 if lease else 0,protocol_m1_response_retries_max=3 if supplicant else 0)
    try:
        owner=os.open(str(LEDGER)+'.workflow',os.O_RDWR|os.O_CREAT|os.O_NOFOLLOW,0o600);fds.append(owner);fcntl.flock(owner,fcntl.LOCK_EX|fcntl.LOCK_NB)
        admission(recovered,lease,supplicant)
        if recovered:report['epoch_id']=Path(str(LEDGER)+'.epoch-id').read_text().strip()
        helper=output/'archer-special-wpa2';shutil.copy2(ROOT/('build/archer-special-dhcp-supplicant-universal' if supplicant else 'build/archer-special-dhcp-lease-universal' if lease else 'build/archer-special-wpa2-recovered-universal' if recovered else 'build/archer-special-wpa2-universal'),helper)
        report.update(history_sha256=digest(SUPPLICANT_HISTORY if supplicant else LEASE_HISTORY if lease else RECOVERED_HISTORY if recovered else HISTORY),helper_sha256=digest(helper),ledger_before_sha256=digest(LEDGER));save(output/'attempt.json',report)
        capture_guard()
        marker=SUPPLICANT_MARKER if supplicant else LEASE_MARKER if lease else RECOVERED_MARKER if recovered else MARKER
        permit=os.open(marker,os.O_RDWR|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600);fds.append(permit);fcntl.flock(permit,fcntl.LOCK_EX|fcntl.LOCK_NB)
        token=b'RECOVERY_RESERVED\n'
        if os.write(permit,token)!=len(token):raise OSError('Incomplete attempt reservation')
        os.fsync(permit)
        parent=os.open(marker.parent,os.O_RDONLY)
        try:os.fsync(parent)
        finally:os.close(parent)
        report['status']='started';save(output/'attempt.json',report)
        env=dict(os.environ,ARCHER_WORKFLOW_FD=str(owner),ARCHER_PROBE_RECOVERY_FD=str(permit),ARCHER_CREDENTIAL_FD=str(credential_fd))
        with (output/'probe.jsonl').open('w') as out,(output/'probe.stderr').open('w') as err:
            result=subprocess.run([str(helper),'--single-special-channel9-wpa2',str(output/'capture')],cwd=ROOT,env=env,pass_fds=(owner,permit,credential_fd),stdout=out,stderr=err,check=False)
        report.update(exit_code=result.returncode,stdout_sha256=digest(output/'probe.jsonl'),stderr_sha256=digest(output/'probe.stderr'),ledger_after_sha256=digest(LEDGER))
        if result.returncode:raise RuntimeError('WPA2 attempt stopped; reservation retained, no retry')
        rows=[json.loads(line) for line in (output/'probe.jsonl').read_text().splitlines()]
        report['validation']=validate_supplicant_result(rows) if supplicant else validate_lease_result(rows) if lease else validate_result(rows,recovered)
        report['status']='completed';save(output/'attempt.json',report)
    except BaseException as error:
        report.update(status='stopped',error=str(error));save(output/'attempt.json',report);raise
    finally:
        for fd in reversed(fds):os.close(fd)
