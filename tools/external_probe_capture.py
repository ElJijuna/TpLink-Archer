"""One tag6 Archer probe during a bounded independent en0 monitor capture."""
import argparse
import json
import os
from pathlib import Path
import struct
import subprocess
import time
from contextlib import nullcontext
import wpa2_once
from passive_workflow import ROOT, save, digest
from special_probe_attempt import execute


def validate_capture_header(header):
    if len(header)!=24 or header[:4]!=b'\xd4\xc3\xb2\xa1' or struct.unpack_from('<I',header,20)[0]!=127:
        raise ValueError('Live classic PCAP with radiotap required')


def validate_live_decode(text):
    lines=text.splitlines()
    if not lines or any(' 2452 MHz' not in line for line in lines):
        raise ValueError('Independent capture has missing or different channel metadata')
    if not any('BSSID:50:92:6a:11:06:66' in line and 'Beacon (Especial)' in line and
               'bad-fcs' not in line for line in lines):
        raise ValueError('Independent receiver has not captured Especial on channel9')


def run(output,phy=False,drained=False,grant=False,station=False,eapol=False,wpa2=False,recovered=False):
    if recovered:wpa2=True
    if eapol:station=True
    if station:phy=True
    if grant:phy=True
    if drained:phy=True
    if os.geteuid()==0:raise ValueError('Run as your normal user after sudo -v; only monitor capture uses sudo')
    output=output.resolve();output.mkdir()
    tag=13 if eapol else 10 if station else 9 if grant else 8 if drained else 7 if phy else 6
    report={'status':'preflight','automatic_retry':False,'tag':tag}
    monitor=None
    credential_context=nullcontext(None)
    if wpa2:
        wpa2_once.admission(recovered)
        credential_context=wpa2_once.credential_pipe()
        report.update(attempt='wpa2_recovered' if recovered else 'wpa2_once',cleanup_report_tag=2 if recovered else 0)
        report.pop('tag',None)
    credential_fd=credential_context.__enter__()
    try:
        with (output/'preflight.jsonl').open('w') as out,(output/'preflight.stderr').open('w') as err:
            subprocess.run([str(ROOT/'build/archer-cleanup-verify-universal'),'--paused-channel9'],cwd=ROOT,stdout=out,stderr=err,check=True,timeout=30)
        # Authentication remains entirely in the user's Terminal.
        subprocess.run(['sudo','-n','true'],check=True)
        with (output/'monitor.stdout').open('w') as out,(output/'monitor.stderr').open('w') as err:
            monitor=subprocess.Popen(['sudo','-n',str(ROOT/'build/archer-monitor-capture-universal'),
                '--capture-probe-channel9',str(output/'monitor')],cwd=ROOT,stdout=out,stderr=err)
            started=time.monotonic()
            def guard():
                if monitor.poll() is not None or time.monotonic()-started>20:
                    raise ValueError('Independent capture no longer live or handoff too late')
                ready=json.loads((output/'monitor/ready.json').read_text())
                if ready.get('channel')!=9 or ready.get('width_mhz')!=20:raise ValueError('Capture not on9/20MHz')
                with (output/'monitor/monitor-channel9.pcap').open('rb') as stream:
                    validate_capture_header(stream.read(24))
                decoded=subprocess.run(['/usr/sbin/tcpdump','-nn','-e','-r',
                    str(output/'monitor/monitor-channel9.pcap')],capture_output=True,text=True,timeout=3)
                # A live file may end in an incomplete last record. Only complete
                # decoded packets are used, and the raw file is retained for audit.
                validate_live_decode(decoded.stdout)
            while True:
                if monitor.poll() is not None:raise RuntimeError('Monitor stopped before handoff')
                try:guard();break
                except (FileNotFoundError,ValueError):
                    if time.monotonic()-started>12:raise
                    time.sleep(0.1)
            time.sleep(1)
            guard()
            report['status']='capture_live';save(output/'paired.json',report)
            print('Independent capture live; submitting the unique WPA2 attempt.' if wpa2 else f'Independent capture live; submitting the unique tag{tag} attempt.',flush=True)
            try:
                if wpa2:wpa2_once.execute(output/'probe',guard,credential_fd,recovered)
                else:execute(output/'probe',external_capture=True,capture_guard=guard,external_phy=phy,external_phy_drained=drained,external_phy_grant=grant,station_exchange=station,eapol_rx=eapol)
                report['probe_executor']='completed'
            except Exception as error:
                report.update(probe_executor='stopped',probe_error=str(error))
            # A terminal TX failure still requires the independent capture evidence.
            monitor.wait(timeout=max(1,55-(time.monotonic()-started)))
            report.update(status='recorded',monitor_exit=monitor.returncode,
                          pcap_sha256=digest(output/'monitor/monitor-channel9.pcap'))
            print(json.dumps(report,indent=2),flush=True)
            print('Reconnect Mac Wi-Fi if needed, then share this output. No automatic probe retry.',flush=True)
    except BaseException as error:
        report.update(status='stopped',error=str(error));raise
    finally:
        credential_context.__exit__(None,None,None)
        # Do not interrupt an in-flight driver cleanup; the monitor self-stops at45s.
        if monitor is not None and monitor.poll() is None:
            try:monitor.wait(timeout=55)
            except subprocess.TimeoutExpired:report['monitor_requires_manual_stop']=True
        save(output/'paired.json',report)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('new_directory',type=Path)
    parser.add_argument('--phy',action='store_true',help='Unique tag7 experiment with before/after PHY counters')
    parser.add_argument('--phy-drained',action='store_true',help='Unique tag8 after recorded pre-context tag7 stop and verified input drain')
    parser.add_argument('--phy-grant',action='store_true',help='Unique tag9 under reversible source v2 WLAN grant policy')
    parser.add_argument('--station-exchange',action='store_true',help='One auth/association/deauth experiment after validated tag9 response')
    parser.add_argument('--eapol-rx',action='store_true',help='Unique tags13-15: associate, observe RSN M1 for up to3s, deauthenticate')
    parser.add_argument('--wpa2-once',action='store_true',help='One WPA2 and encrypted DHCP DISCOVER experiment; hidden local passphrase prompt')
    parser.add_argument('--wpa2-recovered',action='store_true',help='New one-shot WPA2 experiment after verified cold recovery; sequences28–33, fresh epoch report2')
    args=parser.parse_args()
    if args.wpa2_recovered and args.wpa2_once:parser.error('Choose one WPA2 experiment')
    if (args.wpa2_once or args.wpa2_recovered) and any((args.phy,args.phy_drained,args.phy_grant,args.station_exchange,args.eapol_rx)):parser.error('--wpa2-once cannot be combined with prior experiments')
    run(args.new_directory,args.phy,args.phy_drained,args.phy_grant,args.station_exchange,args.eapol_rx,args.wpa2_once,args.wpa2_recovered)
