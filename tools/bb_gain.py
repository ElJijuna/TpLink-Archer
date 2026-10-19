"""Offline RTL8922A/BE structured BB gain decoding; no hardware access."""
import json
import struct
import sys
from pathlib import Path
from firmware_elements import inspect


def decode(pairs, rfe=1):
    fields={};ignored=[];writes=[]
    def store(name,band,bw,path,start,count,value):
        for i in range(count):
            byte=(value>>(8*i))&255
            key=(name,band,bw,path,start+i)
            fields[key]=byte if byte<128 else byte-256
    for index,(address,value) in enumerate(pairs):
        if not 0<=address<=0xffffffff or not 0<=value<=0xffffffff:
            raise ValueError('Invalid gain pair')
        kind=address>>24;band=(address>>16)&255
        bw=(address>>12)&15;path=(address>>8)&15;sub=address&255
        if bw>=2 or band>=12 or path>=2:
            ignored.append(dict(index=index,reason='out_of_range'));continue
        if 0xf9<=address<=0xfe:raise ValueError('Unsupported gain-table flow control')
        if kind==0:
            if sub not in (0,1,2):raise ValueError('Unknown gain-error subtype')
            name,start,count={0:('lna_gain',0,4),1:('lna_gain',4,3),2:('tia_gain',0,2)}[sub]
            store(name,band,bw,path,start,count,value)
        elif kind==1:
            width=sub>>4;part=sub&15
            if width not in (0,1,2,3):raise ValueError('Unknown RPL bandwidth')
            start=4 if width==3 and part else 0
            count=(1,2,4,4)[width]
            store(f'rpl_ofst_{(20,40,80,160)[width]}',band,None,path,start,count,value)
        elif kind==3:
            if sub not in (0,1,2,3):raise ValueError('Unknown OP1DB subtype')
            name,start,count={0:('lna_op1db',0,4),1:('lna_op1db',4,3),
                              2:('tia_lna_op1db',0,4),3:('tia_lna_op1db',4,4)}[sub]
            store(name,band,bw,path,start,count,value)
        elif kind==15:
            writes.append(dict(index=index,bb_address=address&0xfffff,mask=0xffff0000,value=value))
        elif kind==2 or (kind==4 and rfe<50):
            ignored.append(dict(index=index,reason='bypass' if kind==2 else 'external_fem_not_applicable'))
        else:raise ValueError('Unknown BE gain configuration')
    return dict(pair_count=len(pairs),fields=[dict(name=k[0],band=k[1],bw_type=k[2],path=k[3],index=k[4],value=v)
                                           for k,v in sorted(fields.items())],
                ignored=ignored,register_candidates=writes,hardware_writes_performed=False)


def inspect_gain(data,aid,rfe):
    candidates=inspect(data,rfe,aid)['register_candidates']
    element=next((e for e in candidates if e['id']==3),None)
    if element is None:raise ValueError('No applicable BB gain table')
    offset=element['payload_offset']
    pairs=list(struct.iter_unpack('<II',data[offset:offset+element['bytes']]))
    return decode(pairs,rfe)


if __name__=='__main__':
    if len(sys.argv)!=4:raise SystemExit('Usage: bb_gain.py MFW.bin AID RFE')
    print(json.dumps(inspect_gain(Path(sys.argv[1]).read_bytes(),int(sys.argv[2],0),int(sys.argv[3],0)),indent=2))
