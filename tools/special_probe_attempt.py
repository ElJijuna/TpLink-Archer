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


def validate_measurement(rows):
    windows=[r for r in rows if r.get('operation')=='probe_ifs_clm_window']
    if [r.get('index') for r in windows]!=[0,1]:raise ValueError('Two ordered IFS windows required')
    for r in windows:
        if r.get('period')!=62500 or r.get('unit_us')!=32 or not 1900<=r.get('elapsed_ms',0)<=2300:
            raise ValueError('Invalid IFS timing/configuration')
        if type(r.get('tx_units')) is not int or not 0<=r['tx_units']<=65535:
            raise ValueError('Invalid IFS TX field')
    if windows[0]['tx_units']!=0:raise ValueError('Baseline TX was not zero')
    if len([r for r in rows if r.get('phase')=='probe_ifs_controls_restored'])!=1:
        raise ValueError('IFS restoration not confirmed')
    return {'windows':windows,'controls_restored':True,'antenna_emission_proven':False}


def validate_control_baseline(record):
    required={'stage':91,'channel':9,'context':'CLEAN','remaining_sequences':215,'calibration_terminals':8,'active_probes_sent':0,'scan_live_equals_replay':True}
    if any(record.get(k)!=v for k,v in required.items()):raise ValueError('Stage91 calibrated passive control baseline required')
    audit=record.get('control_audit',{})
    if audit.get('ack_frames',0)<1 or audit.get('cts_frames',0)<1 or audit.get('bad_control_fcs')!=0:
        raise ValueError('Verified ACK/CTS capture required')
    if not record.get('evidence'):raise ValueError('Raw baseline evidence missing')


def validate_external_history(previous, baseline):
    if (previous.get('stage')!=92 or previous.get('context')!='CLEAN' or
        previous.get('remaining_sequences')!=206 or
        previous.get('tx_reports')!=[dict(operation='single_probe_tx_report',tag=5,state=1,tx_count=1)] or
        previous.get('cleanup',{}).get('network_tx_paused') is not True):
        raise ValueError('Completed cleanup of the unique stage92 terminal failure required')
    if (baseline.get('stage')!=94 or baseline.get('frequencies_mhz')!={'2452':1499} or
        baseline.get('target_beacons_without_bad_fcs_flag',0)<1 or
        baseline.get('archer_tx_submissions')!=0 or baseline.get('capture_exit')!=0):
        raise ValueError('Independent channel9 baseline required')


def execute(output, unicast=False, monitored=False, ack_capture=False, external_capture=False, capture_guard=None):
    if external_capture:
        if capture_guard is None:raise ValueError('Live independent capture guard required')
        ack_capture=True
    if ack_capture:monitored=True
    if monitored:unicast=True
    output=output.resolve();output.mkdir();fds=[]
    tag=6 if external_capture else 5 if ack_capture else 4 if monitored else 3 if unicast else 2
    report={'status':'preflight','ssid':'Especial','channel':9,'tag':tag,'unicast':unicast,'monitored':monitored,'ack_capture':ack_capture,'external_capture':external_capture,'automatic_retry':False}
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
        if LEDGER.read_bytes()!=b'ARCHSEQ\x01'+bytes([1])*(50 if external_capture else 41 if ack_capture else 50 if monitored else 41 if unicast else 32)+bytes(206 if external_capture else 215 if ack_capture else 206 if monitored else 215 if unicast else 224):
            raise ValueError('Sequence history changed since calibration9')
        for path in (ROOT/'artifacts/37ad-0104-context-reservation',Path(str(LEDGER)+'.epoch-transition')):
            if path.read_bytes()!=b'CLEAN\n':raise ValueError('Context/epoch not CLEAN')
        if external_capture:
            previous_path=ROOT/'research/ack-probe-stage92-observation.json'
            baseline_path=ROOT/'research/monitor-baseline-stage94-observation.json'
            previous=json.loads(previous_path.read_text());base=json.loads(baseline_path.read_text())
            validate_external_history(previous,base)
            if previous.get('attempt',{}).get('ledger_after_sha256')!=digest(LEDGER):
                raise ValueError('History changed since stage92 cleanup')
            for name,sha in previous.get('evidence',{}).items():
                path=(ROOT/name).resolve()
                if not path.is_relative_to(ROOT/'artifacts') or digest(path)!=sha:raise ValueError('Stage92 evidence changed')
            if not previous.get('evidence'):raise ValueError('Missing stage92 raw evidence')
            if digest(ROOT/'artifacts/monitor-channel9-baseline/monitor-channel9.pcap')!=base.get('source_sha256'):
                raise ValueError('Independent baseline PCAP changed')
            capture_guard()
            report.update(previous_sha256=digest(previous_path),independent_baseline_sha256=digest(baseline_path))
        elif ack_capture:
            baseline=ROOT/'research/control-rx-stage91-observation.json'
            base=json.loads(baseline.read_text())
            validate_control_baseline(base)
            if base.get('ledger_sha256_after')!=digest(LEDGER):raise ValueError('Channel9 history changed since control capture')
            for name,sha in base['evidence'].items():
                path=(ROOT/name).resolve()
                if not path.is_relative_to(ROOT/'artifacts') or digest(path)!=sha:raise ValueError('Control baseline evidence changed')
            report['control_baseline_sha256']=digest(baseline)
        elif monitored:
            cleanup=ROOT/'artifacts/cleanup-finalization-stage87/cleanup.json'
            finished=json.loads(cleanup.read_text())
            baseline=ROOT/'research/ifs-clm-stage88-observation.json'
            base=json.loads(baseline.read_text())
            if finished.get('status')!='completed' or finished.get('context_after')!='CLEAN' or finished.get('ledger_sha256')!=digest(LEDGER):
                raise ValueError('Verified stage87 cleanup and unchanged history required')
            if base.get('stage')!=88 or base.get('ledger_sha256_after')!=digest(LEDGER) or base.get('result',{}).get('controls_restored') is not True:
                raise ValueError('Stage88 IFS baseline required')
            if len(base.get('windows',[]))!=2 or any(w.get('tx_units')!=0 for w in base['windows']):
                raise ValueError('Two passive zero-TX windows required')
            for name,sha in base.get('evidence',{}).items():
                path=(ROOT/name).resolve()
                if not path.is_relative_to(ROOT/'artifacts') or digest(path)!=sha:raise ValueError('Baseline evidence changed')
            report.update(cleanup_sha256=digest(cleanup),baseline_sha256=digest(baseline))
        elif unicast:
            history=ROOT/'artifacts/special-probe-stage85'
            previous=json.loads((history/'attempt.json').read_text())
            if previous.get('status')!='completed' or previous.get('ledger_after_sha256')!=digest(LEDGER):
                raise ValueError('Completed stage85 and unchanged ledger required')
            if digest(history/'probe.jsonl')!=previous.get('stdout_sha256'):
                raise ValueError('Stage85 transmission evidence changed')
            validate_result([json.loads(x) for x in (history/'probe.jsonl').read_text().splitlines()])
            report['preceding_attempt_sha256']=digest(history/'attempt.json')
        helper=output/'archer-special-probe'
        shutil.copy2(ROOT/('build/archer-special-external-probe-universal' if external_capture else 'build/archer-special-ack-probe-universal' if ack_capture else 'build/archer-special-monitored-probe-universal' if monitored else 'build/archer-special-unicast-probe-universal' if unicast else 'build/archer-special-probe-universal'),helper)
        report.update(calibration_sha256=digest(evidence),ledger_before_sha256=digest(LEDGER),helper_sha256=digest(helper))
        save(output/'attempt.json',report)
        if external_capture:capture_guard()
        marker=ROOT/('artifacts/37ad-0104-special-channel9-external-probe-attempted' if external_capture else 'artifacts/37ad-0104-special-channel9-ack-probe-attempted' if ack_capture else 'artifacts/37ad-0104-special-channel9-monitored-probe-attempted' if monitored else 'artifacts/37ad-0104-special-channel9-unicast-probe-attempted' if unicast else 'artifacts/37ad-0104-special-channel9-probe-attempted')
        permit=os.open(marker,os.O_RDWR|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600);fds.append(permit)
        fcntl.flock(permit,fcntl.LOCK_EX|fcntl.LOCK_NB)
        token=b'RECOVERY_RESERVED\n'
        if os.write(permit,token)!=len(token):raise OSError('Incomplete attempt reservation')
        os.fsync(permit)
        parent=os.open(marker.parent,os.O_RDONLY);os.fsync(parent);os.close(parent)
        report['status']='started';save(output/'attempt.json',report)
        env=dict(os.environ,ARCHER_WORKFLOW_FD=str(owner),ARCHER_PROBE_RECOVERY_FD=str(permit))
        with (output/'probe.jsonl').open('w') as out,(output/'probe.stderr').open('w') as err:
            result=subprocess.run([str(helper),'--single-special-channel9-external-probe' if external_capture else '--single-special-channel9-ack-probe' if ack_capture else '--single-special-channel9-monitored-probe' if monitored else '--single-special-channel9-unicast-probe' if unicast else '--single-special-channel9-probe',str(output/'capture')],
                cwd=ROOT,env=env,pass_fds=(owner,permit),stdout=out,stderr=err,check=False)
        report.update(exit_code=result.returncode,stdout_sha256=digest(output/'probe.jsonl'),
                      stderr_sha256=digest(output/'probe.stderr'),ledger_after_sha256=digest(LEDGER))
        rows=[json.loads(x) for x in (output/'probe.jsonl').read_text().splitlines()]
        if monitored and any(r.get('operation')=='probe_ifs_clm_window' for r in rows):
            report['measurement']=validate_measurement(rows)
        if result.returncode:raise RuntimeError('Probe stopped; reservation retained, no retry')
        report['validation']=validate_result([json.loads(x) for x in (output/'probe.jsonl').read_text().splitlines()],tag)
        report['status']='completed';save(output/'attempt.json',report)
    except BaseException as error:
        report.update(status='stopped',error=str(error));save(output/'attempt.json',report);raise
    finally:
        for fd in reversed(fds):os.close(fd)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('new_directory',type=Path)
    group=parser.add_mutually_exclusive_group()
    group.add_argument('--unicast',action='store_true',help='Single tag3 experiment after stage85')
    group.add_argument('--monitored',action='store_true',help='Single tag4 no-RTS experiment with paired IFS windows')
    group.add_argument('--ack-capture',action='store_true',help='Single tag5 monitored experiment after passive ACK/CTS validation')
    args=parser.parse_args()
    execute(args.new_directory,args.unicast,args.monitored,args.ack_capture)
