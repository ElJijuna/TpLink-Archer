"""Offline, fail-closed validation of repeated TX-power snapshots and bus effects."""
import argparse
import hashlib
import json
from pathlib import Path
import re
from firmware_elements import inspect


def validate_trace(rows):
    transfers=[r for r in rows if r.get('operation') in ('read','write')]
    if not transfers or any(r.get('status')!=0 or r.get('completed_bytes')!=r.get('width') for r in transfers):
        raise ValueError('Missing, short or failed register transfer')
    writes=[r for r in transfers if r['operation']=='write']
    if [r['address'] for r in writes]!=[0x74,0x270,0x270,0x270,0x74]:
        raise ValueError('Unexpected mutation outside WLAN access/XTAL read protocol')
    if [r['value'] for r in writes if r['address']==0x270]!=[0x81000041,0x810000fd,0x810000fe]:
        raise ValueError('Unexpected XTAL operation')
    window=[r['value'] for r in transfers if r['operation']=='read' and r['address']==0x74]
    if len(window)<2 or window[0]!=window[-1]:raise ValueError('WLAN access window not restored')
    snapshots=[r for r in rows if r.get('operation')=='tx_power_snapshot']
    if len(snapshots)!=210 or len({r['address'] for r in snapshots})!=210:
        raise ValueError('Incomplete or duplicate power snapshot')
    if any(not r.get('matches_host_plan') or (r['observed']&r['mask'])!=r['expected_host_plan'] for r in snapshots):
        raise ValueError('Hardware differs from host plan')
    counters=[r for r in rows if r.get('operation')=='passive_phy_tx_counters']
    if [r.get('sample') for r in counters]!=[0,1,2] or any(r.get('controls_enabled') is not True for r in counters):
        raise ValueError('Three enabled PHY counter samples required')
    if any(r['counts']!=counters[0]['counts'] for r in counters):raise ValueError('Passive TX counters changed')
    terminal=[r for r in rows if r.get('result')=='tx_power_snapshot_complete']
    if terminal!=[dict(result='tx_power_snapshot_complete',registers=210,differences_from_host_plan=0,
                      network_tx_paused=True,h2c_submissions=0,firmware_resets=0)]:
        raise ValueError('Missing completed paused snapshot')
    return {r['address']:r['observed']&r['mask'] for r in snapshots}


def validate_reference(snapshot, source, firmware):
    region=source.split('static int rtw8922a_ctrl_tx_path_tmac',1)[1].split('int ret = 0;',1)[0]
    pairs=[(int(a,16),int(v,16)) for a,v in re.findall(r'\{(0x[0-9A-Fa-f]+),\s*(0x[0-9A-Fa-f]+)\}',region)]
    if len(pairs)!=7 or any(snapshot.get(a)!=v for a,v in pairs):raise ValueError('AB route differs from reference source')
    if hashlib.sha256(firmware).hexdigest()!='4340bc322912f30f1793bb57a1747c23b15e40db2df628d6687d1ddf722f2828':
        raise ValueError('Unbound firmware')
    element=next(e for e in inspect(firmware,1,0x6933)['power_candidates'] if e['id']==9)
    if element['entry_size']!=11:raise ValueError('Unexpected byrate layout')
    values={}
    for offset in range(element['payload_offset'],element['payload_offset']+element['bytes'],11):
        entry=firmware[offset:offset+11]
        band,nss,rate_set,shift,length=entry[:5]
        if band==0 and rate_set==0 and entry[9]==0:
            if length>4:raise ValueError('Invalid byrate CCK extent')
            for i in range(length):
                value=entry[5+i];values[shift+i]=(value-256 if value>=128 else value)//2
    if set(values)!=set(range(4)):raise ValueError('Incomplete CCK byrate firmware cells')
    packed=sum((values[i]&255)<<(8*i) for i in range(4))
    if snapshot.get(0x11e00)!=packed:raise ValueError('Resident CCK rates differ from firmware cells')
    return dict(ab_path_words=7,firmware_cck_rate_cells=4,firmware_cck_word=packed)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('logs',nargs=3,type=Path)
    args=parser.parse_args();snapshots=[validate_trace([json.loads(x) for x in p.read_text().splitlines()]) for p in args.logs]
    if any(s!=snapshots[0] for s in snapshots):raise ValueError('Repeated masked register snapshots differ')
    root=Path(__file__).resolve().parents[1]
    reference=validate_reference(snapshots[0],(root/'artifacts/rtw8922a.c').read_text(),(root/'artifacts/rtw8922a_fw-1.bin').read_bytes())
    print(json.dumps(dict(result='validated',repeated_snapshots=3,stable_registers=210,
        wlan_window_restored=True,only_wlan_and_xtal_read_protocol_writes=True,
        independent_reference=reference,antenna_emission_proven=False),indent=2))
