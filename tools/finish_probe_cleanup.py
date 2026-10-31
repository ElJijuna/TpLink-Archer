"""Finalize only stage86's fully acknowledged failed probe; no reset, retry or TX."""
import argparse
import fcntl
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
from passive_workflow import ROOT, LEDGER, save, digest


def validate_observation(record):
    if record.get('stage')!=86 or record.get('context')!='DIRTY':raise ValueError('Stage86 failed context required')
    attempt=record.get('attempt',{})
    if attempt.get('status')!='stopped' or attempt.get('exit_code')!=2 or attempt.get('tag')!=3 or attempt.get('unicast') is not True:
        raise ValueError('Not the terminal failed unicast experiment')
    if record.get('tx_reports')!=[dict(operation='single_probe_tx_report',tag=3,state=1,tx_count=1)]:
        raise ValueError('Unambiguous retry-limit terminal required')
    if record.get('cleanup_steps_6_7_8_acknowledged') is not True or record.get('callbacks_drained') is not True:
        raise ValueError('Cleanup/drain evidence incomplete')
    read=record.get('last_scheduler_read',{})
    if read.get('operation')!='read' or read.get('address')!=0x10398 or read.get('status')!=0 or read.get('completed_bytes')!=4 or read.get('value',-1)&0x3ffff:
        raise ValueError('Paused scheduler evidence missing')
    if not record.get('evidence'):raise ValueError('Missing raw evidence')


def execute(output):
    output=output.resolve();output.mkdir();fds=[]
    report={'status':'preflight','stage':87,'h2c_submissions':0,'firmware_resets':0,'new_probe_attempts':0}
    try:
        owner=os.open(str(LEDGER)+'.workflow',os.O_RDWR|os.O_CREAT|os.O_NOFOLLOW,0o600);fds.append(owner)
        fcntl.flock(owner,fcntl.LOCK_EX|fcntl.LOCK_NB)
        observation=ROOT/'research/special-unicast-stage86-observation.json'
        record=json.loads(observation.read_text());validate_observation(record)
        for name,sha in record['evidence'].items():
            path=(ROOT/name).resolve()
            if not path.is_relative_to(ROOT/'artifacts') or digest(path)!=sha:raise ValueError('Raw evidence changed')
        expected=record['attempt']['ledger_after_sha256']
        if digest(LEDGER)!=expected:raise ValueError('Ledger changed after failed attempt')
        if Path(str(LEDGER)+'.epoch-transition').read_bytes()!=b'CLEAN\n':raise ValueError('Epoch not CLEAN')
        context=ROOT/'artifacts/37ad-0104-context-reservation'
        fd=os.open(context,os.O_RDWR|os.O_NOFOLLOW);fds.append(fd)
        info=os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_nlink!=1:raise ValueError('Invalid context reservation')
        fcntl.flock(fd,fcntl.LOCK_EX|fcntl.LOCK_NB)
        if os.pread(fd,7,0)!=b'DIRTY\n':raise ValueError('Expected original DIRTY context')
        (output/'previous-context.bin').write_bytes(b'DIRTY\n')
        report.update(observation_sha256=digest(observation),ledger_sha256=expected)
        save(output/'cleanup.json',report)
        env=dict(os.environ,ARCHER_WORKFLOW_FD=str(owner))
        for name,filename,args,terminal in (
            ('replay','archer-context-cleanup-replay-universal',['--stage86',str(ROOT/'artifacts/special-unicast-probe-stage86/capture')],'stage86_cleanup_replayed'),
            ('verify','archer-cleanup-verify-universal',['--paused-channel9'],'paused_channel9_verified')):
            helper=output/filename;shutil.copy2(ROOT/'build'/filename,helper)
            report[name+'_helper_sha256']=digest(helper)
            with (output/(name+'.jsonl')).open('w') as out,(output/(name+'.stderr')).open('w') as err:
                result=subprocess.run([str(helper),*args],cwd=ROOT,env=env,pass_fds=(owner,),stdout=out,stderr=err,check=False)
            report[name+'_exit']=result.returncode
            report[name+'_stdout_sha256']=digest(output/(name+'.jsonl'))
            report[name+'_stderr_sha256']=digest(output/(name+'.stderr'))
            save(output/'cleanup.json',report)
            if result.returncode:raise RuntimeError(name+' failed; context remains DIRTY')
            rows=[json.loads(x) for x in (output/(name+'.jsonl')).read_text().splitlines()]
            matches=[r for r in rows if r.get('result')==terminal]
            if len(matches)!=1:raise ValueError('Missing unique '+terminal)
            report[name+'_result']=matches[0]
        if report['replay_result']!=dict(result='stage86_cleanup_replayed',issued_commands=9,final_step=9,retry_limit_terminal=True,pending_h2c=False):
            raise ValueError('Incomplete replay proof')
        if report['verify_result']!=dict(result='paused_channel9_verified',network_tx_paused=True,h2c_submissions=0,firmware_resets=0,mcc_slot=0):
            raise ValueError('Incomplete live verification')
        if digest(LEDGER)!=expected or os.pread(fd,7,0)!=b'DIRTY\n':raise ValueError('Ownership state changed during verification')
        report['status']='verified_before_commit';save(output/'cleanup.json',report)
        if os.pwrite(fd,b'CLEAN\n',0)!=6:raise OSError('Incomplete CLEAN write')
        os.fsync(fd)
        report.update(status='completed',context_after='CLEAN',channel=9)
        save(output/'cleanup.json',report)
    except BaseException as error:
        report.update(status='stopped',error=str(error));save(output/'cleanup.json',report);raise
    finally:
        for fd in reversed(fds):os.close(fd)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('new_directory',type=Path)
    execute(parser.parse_args().new_directory)
