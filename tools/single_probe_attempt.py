"""One explicit probe attempt following a completed cold workflow; never auto-retries."""
import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
from passive_workflow import ROOT, LEDGER, save, digest, plan


def validate_cold(record):
    if record.get('status') != 'completed' or record.get('mode') != 'cold-start':
        raise ValueError('Completed cold-start workflow required')
    expected=['inspect','boot','radio','initial_rfk','tx_rfk','scan','sweep']
    if [p.get('name') for p in record.get('phases',[])] != expected:
        raise ValueError('Full cold RFK and sweep history required')
    if any(p.get('status')!='completed' or p.get('exit_code')!=0 for p in record['phases']):
        raise ValueError('Cold workflow contains an incomplete phase')
    for phase, (_, _, terminal) in zip(record['phases'], plan('cold-start', True)):
        result=phase.get('result',{})
        if result.get('result')!=terminal and result.get('phase')!=terminal:
            raise ValueError('Missing cold phase terminal evidence')
    if record['phases'][-1]['result'].get('network_tx_paused') is not True:
        raise ValueError('Cold workflow did not confirm paused TX')
    if record.get('sweep_report',{}).get('live_equals_replay') is not True:
        raise ValueError('Cold sweep replay validation required')


def validate_probe(rows):
    writes=[r for r in rows if r.get('operation')=='single_probe_usb_write']
    reports=[r for r in rows if r.get('operation')=='single_probe_tx_report']
    cleanup=[r for r in rows if r.get('result')=='context_cleanup_acknowledged']
    drains=[r for r in rows if r.get('operation')=='bulk_async_drained']
    if len(writes)!=1 or any(writes[0].get(k)!=v for k,v in {'endpoint':5,'bytes':96,'status':0}.items()):
        raise ValueError('Expected exactly one successful96-byte USB probe submission')
    if len(reports)!=1 or any(reports[0].get(k)!=v for k,v in {'tag':1,'state':0,'tx_count':1}.items()):
        raise ValueError('Expected one TX_DONE report with count1')
    if len(cleanup)!=1 or cleanup[0].get('network_tx_paused') is not True:
        raise ValueError('Context cleanup and paused TX not confirmed')
    if len(drains)!=1 or drains[0].get('pending')!=0:
        raise ValueError('USB callbacks not drained')
    return {'firmware_tx_done':True,'reported_tx_count':1,'usb_submissions':1,
            'context_cleanup_acknowledged':True,'network_tx_paused':True,
            'external_ack_verified':False,'association':False}


def execute(cold, output):
    cold=cold.resolve();output=output.resolve();output.mkdir()
    record={'status':'preflight','automatic_retry':False}
    report=output/'attempt.json';fds=[]
    try:
        owner=os.open(str(LEDGER)+'.workflow',os.O_RDWR|os.O_CREAT|os.O_NOFOLLOW,0o600);fds.append(owner)
        fcntl.flock(owner,fcntl.LOCK_EX|fcntl.LOCK_NB)
        manifest=cold/'workflow.json';validated=json.loads(manifest.read_text());validate_cold(validated)
        if validated.get('ledger_sha256_after')!=digest(LEDGER):
            raise ValueError('Ledger changed since cold workflow; no attempt')
        for suffix in ('.epoch-transition',):
            if Path(str(LEDGER)+suffix).read_bytes()!=b'CLEAN\n':raise ValueError('Epoch is not CLEAN')
        if (ROOT/'artifacts/37ad-0104-context-reservation').read_bytes()!=b'CLEAN\n':
            raise ValueError('Context is not CLEAN')
        marker=ROOT/'artifacts/37ad-0104-single-probe-attempted'
        if marker.is_symlink() or marker.read_bytes()!=b'TAG1 RESERVED\n':
            raise ValueError('Expected original single-probe marker')
        shutil.copyfile(marker,output/'previous-probe-marker.bin')
        with (output/'previous-probe-marker.bin').open('rb') as archive:os.fsync(archive.fileno())
        helper=output/'archer-single-probe';shutil.copy2(ROOT/'build/archer-single-probe-universal',helper)
        cold_hash=digest(manifest)
        record.update(cold_workflow_sha256=cold_hash,helper_sha256=digest(helper))
        save(report,record)
        # This cold history can authorize at most one attempt, including a crash.
        used=ROOT/('artifacts/probe-cold-history-'+cold_hash+'.used')
        fd=os.open(used,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600);fds.append(fd)
        os.write(fd,b'RESERVED\n');os.fsync(fd)
        temporary=marker.with_name(marker.name+'.recovery-reserved')
        permit=os.open(temporary,os.O_RDWR|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600);fds.append(permit)
        fcntl.flock(permit,fcntl.LOCK_EX|fcntl.LOCK_NB)
        os.write(permit,b'RECOVERY_RESERVED\n');os.fsync(permit)
        os.replace(temporary,marker)
        directory=os.open(marker.parent,os.O_RDONLY);os.fsync(directory);os.close(directory)
        record['status']='started';save(report,record)
        env=dict(os.environ,ARCHER_WORKFLOW_FD=str(owner),ARCHER_PROBE_RECOVERY_FD=str(permit))
        with (output/'probe.jsonl').open('w') as out,(output/'probe.stderr').open('w') as err:
            result=subprocess.run([str(helper),'--single-channel1-probe',str(output/'capture')],cwd=ROOT,
                                  env=env,pass_fds=(owner,permit),stdout=out,stderr=err,check=False)
        if result.returncode==0:
            record['validation']=validate_probe([json.loads(line) for line in (output/'probe.jsonl').read_text().splitlines()])
        record.update(exit_code=result.returncode,status='completed' if result.returncode==0 else 'stopped',
                      stdout_sha256=digest(output/'probe.jsonl'),stderr_sha256=digest(output/'probe.stderr'))
        save(report,record)
        if result.returncode:raise RuntimeError('Probe attempt failed; marker retained, no retry')
    except BaseException as error:
        record.update(status='stopped',error=str(error));save(report,record);raise
    finally:
        for fd in reversed(fds):os.close(fd)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('completed_cold_workflow',type=Path);parser.add_argument('new_directory',type=Path)
    args=parser.parse_args();execute(args.completed_cold_workflow,args.new_directory)
