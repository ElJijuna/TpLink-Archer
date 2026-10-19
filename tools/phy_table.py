"""Offline RTL8922A PHY table selection; produces data, never USB/RF writes."""
import json
import struct
import sys
from pathlib import Path
from firmware_elements import inspect


def select_headline(pairs, rfe, cv):
    if not pairs or not 0<=rfe<=255 or not 0<=cv<=255:
        raise ValueError('Empty table or invalid RFE/CV')
    headlines=[]
    for address,_ in pairs:
        if address>>28 != 15:break
        headlines.append(address & 0xfffffff)
    if not headlines:return 0,None
    for target in ((rfe<<16)|cv,(rfe<<16)|255):
        if target in headlines:return len(headlines),target
    # Source chooses the last maximum CV on ties.
    for wanted in (rfe,255):
        choices=[x for x in headlines if (x>>16)&255==wanted]
        if choices:return len(headlines),max(enumerate(choices),key=lambda item:(item[1]&255,item[0]))[1]
    raise ValueError('No applicable PHY headline')


def interpret(pairs, rfe, cv, mode):
    if mode not in ('bb','rf_v1'):raise ValueError('Unsupported PHY callback')
    start,selected=select_headline(pairs,rfe,cv)
    matched=True;found=False;target=None;in_branch=False;pending=False
    operations=[]
    for index,(address,value) in enumerate(pairs[start:],start):
        if not 0<=address<=0xffffffff or not 0<=value<=0xffffffff:
            raise ValueError('Invalid register pair')
        condition=address>>28
        if condition in (8,9):
            if selected is None:raise ValueError('Conditional table has no headline')
            if pending or (condition==8 and in_branch) or (condition==9 and not in_branch):
                raise ValueError('Invalid or nested PHY branch')
            in_branch=True;pending=True;target=address&0xfffffff
            continue
        if condition==4:
            if not in_branch or not pending:raise ValueError('CHECK without branch target')
            pending=False
            matched=not found and target==selected
            if matched:found=True
            continue
        if condition==10:
            if not in_branch or pending or not found:
                raise ValueError('ELSE without selected PHY branch')
            matched=False
            continue
        if condition==11:
            if not in_branch or pending:raise ValueError('END without complete branch')
            in_branch=False;matched=True;found=False
            continue
        if condition not in (0,):raise ValueError('Unsupported table opcode')
        if pending:raise ValueError('PHY data before CHECK')
        if not matched:continue
        operation=dict(pair_index=index,address=address,value=value)
        if mode=='bb' and address in range(0xf9,0xff):
            operation.update(kind='delay',microseconds={0xfe:50000,0xfd:5000,0xfc:1000,0xfb:50,0xfa:5,0xf9:1}[address])
        elif mode=='rf_v1' and address==0xfe:
            operation.update(kind='delay',microseconds=50000)
        elif mode=='bb' and value==0xbabecafe:
            operation.update(kind='bypass')
        else:
            operation.update(kind='bb_write' if mode=='bb' else 'rf_write',
                             rf_firmware_store=mode=='rf_v1' and address>=0x100)
        operations.append(operation)
    if in_branch or pending:raise ValueError('Unterminated PHY branch')
    return dict(headline_pairs=start,selected_target=selected,operations=operations,
                selected_pairs=len(operations),delay_microseconds=sum(x.get('microseconds',0) for x in operations))


def plan(data, aid, rfe, cv):
    inventory=inspect(data,rfe,aid)
    tables=[]
    for element in inventory['register_candidates']:
        if element['id']==3:continue # Gain tables use a structured gain decoder, not BB writes.
        offset=element['payload_offset']
        pairs=list(struct.iter_unpack('<II',data[offset:offset+element['bytes']]))
        mode='rf_v1' if element['id'] in (4,5,6,7) else 'bb'
        decoded=interpret(pairs,rfe,cv,mode)
        tables.append(dict(name=element['name'],target=element['target'],mode=mode,**decoded))
    return dict(firmware_sha256=inventory['sha256'],aid=aid,rfe=rfe,cv=cv,tables=tables,
                executable=False,hardware_writes_performed=False,
                limitations=['No MAC/BB power initialization', 'No gain table decoder',
                             'No RF access or firmware table-store transport',
                             'No firmware/runtime compatibility validation'])


if __name__=='__main__':
    if len(sys.argv)!=5:raise SystemExit('Usage: phy_table.py MFW.bin AID RFE CV')
    print(json.dumps(plan(Path(sys.argv[1]).read_bytes(),*(int(x,0) for x in sys.argv[2:])),indent=2))
