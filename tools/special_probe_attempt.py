"""One SSID-directed Especial probe after the recorded stage84 channel9 calibration."""
import argparse
import fcntl
import json
import os
from pathlib import Path
import shutil
import subprocess
from passive_workflow import ROOT, LEDGER, save, digest


def validate_calibration(record):
    required={'stage':84,'channel':9,'frequency_mhz':2452,'mcc_slot':0,
              'terminal_success_count':8,'context':'CLEAN','network_tx_paused':True,
              'available_sequences':224,'new_network_frame_submissions':0}
    if any(record.get(k)!=v for k,v in required.items()):
        raise ValueError('Completed stage84 channel9 calibration required')
    if [x.get('sequence') for x in record.get('commands',[])]!=list(range(24,32)):
        raise ValueError('Unexpected calibration sequence history')
    if [x.get('rfk_command') for x in record['commands']]!=[8,0,8,4,1,0,3,6]:
        raise ValueError('Incomplete calibration command history')
    if record.get('rxdck_replay')!={'rxdck_clean':True,'channels':[9,9],'timeouts':[0,0]}:
        raise ValueError('Clean channel9 RXDCK required')
    if not record.get('evidence'):raise ValueError('Missing calibration evidence')


def validate_result(rows, tag=2):
    writes=[r for r in rows if r.get('operation')=='single_probe_usb_write']
    reports=[r for r in rows if r.get('operation')=='single_probe_tx_report']
    cleanup=[r for r in rows if r.get('result')=='context_cleanup_acknowledged']
    drains=[r for r in rows if r.get('operation')=='bulk_async_drained']
    if writes!=[{'operation':'single_probe_usb_write','endpoint':5,'bytes':104,'status':0}]:
        raise ValueError('Expected exactly one104-byte USB submission')
    if reports!=[{'operation':'single_probe_tx_report','tag':tag,'state':0,'tx_count':1}]:
        raise ValueError('Expected unique tag2 TX_DONE/count1')
    if len(cleanup)!=1 or cleanup[0].get('network_tx_paused') is not True:
        raise ValueError('Context cleanup missing')
    if drains!=[{'operation':'bulk_async_drained','pending':0}]:raise ValueError('USB not drained')
    return {'firmware_tx_done':True,'usb_submissions':1,'external_response_verified':False}


def execute(output, unicast=False):
    output=output.resolve();output.mkdir();fds=[]
    report={'status':'preflight','ssid':'Especial','channel':9,'tag':3 if unicast else 2,'unicast':unicast,'automatic_retry':False}
    try:
        owner=os.open(str(LEDGER)+'.workflow',os.O_RDWR|os.O_CREAT|os.O_NOFOLLOW,0o600);fds.append(owner)
        fcntl.flock(owner,fcntl.LOCK_EX|fcntl.LOCK_NB)
        evidence=ROOT/'research/channel9-calibration-stage84-observation.json'
        record=json.loads(evidence.read_text());validate_calibration(record)
        for name,sha in record['evidence'].items():
            path=(ROOT/name).resolve()
            if not path.is_relative_to(ROOT/'artifacts') or digest(path)!=sha:
                raise ValueError('Calibration evidence changed')
        # Fresh verified epoch had0..31 reserved after stage84. Any later H2C
        # invalidates this one-shot admission, rather than guessing current history.
        if LEDGER.read_bytes()!=b'ARCHSEQ\x01'+bytes([1])*(41 if unicast else 32)+bytes(215 if unicast else 224):
            raise ValueError('Sequence history changed since calibration9')
        for path in (ROOT/'artifacts/37ad-0104-context-reservation',Path(str(LEDGER)+'.epoch-transition')):
            if path.read_bytes()!=b'CLEAN\n':raise ValueError('Context/epoch not CLEAN')
        if unicast:
            history=ROOT/'artifacts/special-probe-stage85'
            previous=json.loads((history/'attempt.json').read_text())
            if previous.get('status')!='completed' or previous.get('ledger_after_sha256')!=digest(LEDGER):
                raise ValueError('Completed stage85 and unchanged ledger required')
            if digest(history/'probe.jsonl')!=previous.get('stdout_sha256'):
                raise ValueError('Stage85 transmission evidence changed')
            validate_result([json.loads(x) for x in (history/'probe.jsonl').read_text().splitlines()])
            report['preceding_attempt_sha256']=digest(history/'attempt.json')
        helper=output/'archer-special-probe'
        shutil.copy2(ROOT/('build/archer-special-unicast-probe-universal' if unicast else 'build/archer-special-probe-universal'),helper)
        report.update(calibration_sha256=digest(evidence),ledger_before_sha256=digest(LEDGER),helper_sha256=digest(helper))
        save(output/'attempt.json',report)
        marker=ROOT/('artifacts/37ad-0104-special-channel9-unicast-probe-attempted' if unicast else 'artifacts/37ad-0104-special-channel9-probe-attempted')
        permit=os.open(marker,os.O_RDWR|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600);fds.append(permit)
        fcntl.flock(permit,fcntl.LOCK_EX|fcntl.LOCK_NB)
        token=b'RECOVERY_RESERVED\n'
        if os.write(permit,token)!=len(token):raise OSError('Incomplete attempt reservation')
        os.fsync(permit)
        parent=os.open(marker.parent,os.O_RDONLY);os.fsync(parent);os.close(parent)
        report['status']='started';save(output/'attempt.json',report)
        env=dict(os.environ,ARCHER_WORKFLOW_FD=str(owner),ARCHER_PROBE_RECOVERY_FD=str(permit))
        with (output/'probe.jsonl').open('w') as out,(output/'probe.stderr').open('w') as err:
            result=subprocess.run([str(helper),'--single-special-channel9-unicast-probe' if unicast else '--single-special-channel9-probe',str(output/'capture')],
                cwd=ROOT,env=env,pass_fds=(owner,permit),stdout=out,stderr=err,check=False)
        report.update(exit_code=result.returncode,stdout_sha256=digest(output/'probe.jsonl'),
                      stderr_sha256=digest(output/'probe.stderr'),ledger_after_sha256=digest(LEDGER))
        if result.returncode:raise RuntimeError('Probe stopped; reservation retained, no retry')
        report['validation']=validate_result([json.loads(x) for x in (output/'probe.jsonl').read_text().splitlines()],3 if unicast else 2)
        report['status']='completed';save(output/'attempt.json',report)
    except BaseException as error:
        report.update(status='stopped',error=str(error));save(output/'attempt.json',report);raise
    finally:
        for fd in reversed(fds):os.close(fd)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('new_directory',type=Path)
    parser.add_argument('--unicast',action='store_true',help='Single new tag3 ACK-eligible experiment after stage85')
    args=parser.parse_args()
    execute(args.new_directory,args.unicast)
