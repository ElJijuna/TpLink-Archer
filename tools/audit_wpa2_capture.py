"""Offline WPA2 capture audit. No USB access, transmission, sudo or key files."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
from passive_workflow import ROOT, save, digest
from wpa2_once import credential_pipe


def run(capture, output):
    if os.geteuid()==0:raise ValueError('Run as your normal user; no sudo is needed')
    capture=capture.resolve();output=output.resolve()
    if not capture.is_relative_to(ROOT/'artifacts') or not capture.is_dir():raise ValueError('Recorded artifact capture required')
    for name in ('secure-tx-2.bin','secure-tx-3.bin','secure-tx-4.bin'):
        if not (capture/name).is_file():raise ValueError('Recorded M2, M4 and encrypted DISCOVER required')
    if not output.is_relative_to(ROOT/'artifacts'):raise ValueError('New artifact output directory required')
    output.mkdir()
    helper=output/'archer-wpa2-capture-audit';shutil.copy2(ROOT/'build/archer-wpa2-capture-audit-universal',helper)
    report=dict(status='prepared',offline=True,h2c_submissions=0,network_submissions=0,helper_sha256=digest(helper),capture=str(capture),input_sha256={p.name:digest(p) for p in capture.iterdir() if p.is_file()})
    save(output/'audit.json',report)
    try:
        with credential_pipe() as fd,(output/'audit.jsonl').open('w') as out,(output/'audit.stderr').open('w') as err:
            result=subprocess.run([str(helper),str(capture)],env=dict(os.environ,ARCHER_CREDENTIAL_FD=str(fd)),pass_fds=(fd,),cwd=ROOT,stdout=out,stderr=err,check=False,timeout=30)
        report.update(exit_code=result.returncode,stdout_sha256=digest(output/'audit.jsonl'),stderr_sha256=digest(output/'audit.stderr'))
        if result.returncode:raise RuntimeError('Offline audit stopped; see audit.stderr; no radio action occurred')
        rows=[json.loads(x) for x in (output/'audit.jsonl').read_text().splitlines()]
        summaries=[r for r in rows if r.get('result')=='offline_wpa2_audited']
        if len(summaries)!=1:raise ValueError('Missing unique audit summary')
        report.update(status='completed',summary=summaries[0]);save(output/'audit.json',report)
        for row in rows:print(json.dumps(row),flush=True)
    except BaseException as error:
        report.update(status='stopped',error=str(error));save(output/'audit.json',report);raise


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture_directory',type=Path);parser.add_argument('new_output_directory',type=Path)
    args=parser.parse_args()
    try:run(args.capture_directory,args.new_output_directory)
    except (ValueError,OSError,RuntimeError) as error:parser.exit(2,str(error)+'\n')
