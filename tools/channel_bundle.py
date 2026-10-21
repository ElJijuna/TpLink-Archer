"""Bind channel1 gain and trim inputs to exact firmware and captured board efuse."""
import hashlib
from pathlib import Path
from bb_gain import inspect_gain
from phy_calibration import decode
ROOT=Path(__file__).resolve().parents[1]
def bound(path,digest):
    data=(ROOT/path).read_bytes()
    if hashlib.sha256(data).hexdigest()!=digest:raise ValueError('Channel input hash mismatch: '+path)
    return data
fw=bound('artifacts/rtw8922a_fw-1.bin','4340bc322912f30f1793bb57a1747c23b15e40db2df628d6687d1ddf722f2828')
rf=bound('artifacts/efuse-stage13/rf-logical.bin','f105aa4a4efd345b6533da26d6dfb250bda461533a8ca962e27ad3083600fc37')
phy=bound('artifacts/efuse-stage13/phycap.bin','ca1de3fdd127c861ab5f3b8fccefeef916a109f437b7de50e23f5ea08486bfd2')
if len(rf)!=0x240 or rf[0xca]!=1:raise ValueError('Unexpected RF map or RFE')
fields=inspect_gain(fw,0x6933,1)['fields']
lines=['#pragma once','#include "archer/channel_power.hpp"','inline const archer::Channel1BbInputs trusted_channel1_bb=[] {','archer::Channel1BbInputs b;']
for path in range(2):
    for member,name,count in [('lna_gain','lna_gain',7),('tia_gain','tia_gain',2),('lna_op1db','lna_op1db',7),('tia_lna_op1db','tia_lna_op1db',8),('rpl160','rpl_ofst_160',8),('rpl80','rpl_ofst_80',4),('rpl40','rpl_ofst_40',2),('rpl20','rpl_ofst_20',1)]:
        for index in range(count):
            matches=[f['value'] for f in fields if f['name']==name and f['band']==0 and f['path']==path and f['index']==index and f['bw_type']==(None if member.startswith('rpl') else 0)]
            if not matches and member in ('rpl80','rpl160'):
                # Source fresh ieee80211_alloc_hw private state is zero initialized;
                # selected 2G firmware gain table omits unsupported 80/160 MHz cells.
                matches=[0]
            if len(matches)!=1:raise ValueError('Missing or ambiguous BB gain cell')
            lines.append(f'b.gain[{path}].{member}[{index}]={matches[0]};')
# Full 13-band set controls Linux offset_valid, not only the two used here.
raw=[rf[0xd4+p*5+j] for p in range(2) for j in range(5)]+[rf[0x1a8+p*8+j] for p in range(2) for j in range(8)]
valid=not all(x==255 for x in raw) and not all(x==0 for x in raw)
lines.append('b.efuse_offset_valid='+str(valid).lower()+';')
def gain(v):
    v=((v^0x7f)+1)&255 if v&128 else v
    return v-256 if v>=128 else v
for p in range(2):
    lines.append(f'b.efuse_cck[{p}]={gain(rf[0xd5+p*5])};b.efuse_ofdm[{p}]={gain(rf[0xd4+p*5])};')
lines+=['return b;}();','inline const archer::Channel1BoardTrim trusted_channel1_trim=[] {','archer::Channel1BoardTrim t;']
t=decode(phy)
lines += ['t.pa_and_pad_programmed='+str(t['pa_and_pad_programmed']).lower()+';','t.thermal_programmed='+str(t['thermal_programmed']).lower()+';']
for p in t['paths']:
    i=p['path'];lines.append(f't.pa_raw[{i}]={p["pa_raw"]};t.pad_raw[{i}]={p["pad_raw"]};t.thermal_trim[{i}]={p["thermal_trim"]};')
lines+=['return t;}();']
(ROOT/'artifacts/channel_payload.hpp').write_text('\n'.join(lines)+'\n')
print('Bound channel1 gain and board trim generated')

# Explicit fresh world/unconfigured diagnostic profile, with scheduler TX paused.
# Mirrors private-state zero initialization plus firmware loading, not a country claim.
import struct
from firmware_elements import inspect
power={e['id']:e for e in inspect(fw,1,0x6933)['power_candidates']}
def entries(ident,size):
    e=power[ident]
    if e['entry_size']!=size:raise ValueError('Unexpected power element size')
    return [fw[i:i+size] for i in range(e['payload_offset'],e['payload_offset']+e['bytes'],size)]
def signed(v):return v-256 if v>=128 else v
# rtw89 private byr arrays are zero-initialized; loader overwrites listed cells.
byr={}
for e in entries(9,11):
    band,nss,rs,shift,length=e[:5];bw,ofdma=e[9:11]
    if band>=3 or bw>=5 or rs>=5 or length>4:raise ValueError('Unexpected byrate entry')
    for i in range(length):
        # CCK/OFDM/OFFSET storage has no NSS/OFDMA dimension.
        byr[(band,bw,rs,ofdma if rs in (2,3) else 0,nss if rs in (2,3) else 0,shift+i)]=signed(e[5+i])
def getby(bw,rs,ofdma,nss,index):
    return byr.get((0,bw,rs,ofdma if rs in (2,3) else 0,nss if rs in (2,3) else 0,index),0)//2
ops=[]
def write(addr,value,mask=0xffffffff):ops.append((addr,mask,value&mask))
def pack(values):return sum((v&255)<<(8*i) for i,v in enumerate(values))
addr=0x11e00
for bw in range(5):
    for nss in range(2):
        out=[]
        specs=[(0,0,0,4,True),(1,0,0,8,True),(2,0,14,2,True),(2,1,14,2,True),(2,0,0,14,False),(3,0,0,4,False),(2,1,0,14,False),(3,1,0,4,False)]
        for rs,ofdma,start,count,single in specs:
            if nss and single:continue
            if bw>1 and rs==0:continue
            out += [getby(bw,rs,ofdma,nss,i) for i in range(start,start+count)]
        if len(out)%4:raise ValueError('Byrate packing not aligned')
        for i in range(0,len(out),4):write(addr,pack(out[i:i+4]));addr+=4
# Source enum offsets HE,VHT,HT,OFDM,CCK,DLRU_EHT,DLRU_HE,EHT.
write(0x11a30,sum((getby(0,4,0,0,index)&15)<<(4*i) for i,index in enumerate((4,3,2,1,0,7,6,5))))
shapes={}
for e in entries(16,4):shapes[tuple(e[:3])]=e[3]
# Firmware omits WW TX shape: source zero-initialized RFE data retains 0.
write(0x26c04,1 if shapes.get((0,1,0),0) else 0,3)
lmt={tuple(e[:6]):signed(e[6]) for e in entries(10,7)}
ru={tuple(e[:4]):signed(e[4]) for e in entries(13,5)}
addr=0x11fac
for nt in range(2):
    # Source memset page then fill four pairs at 20MHz; 76-byte page.
    page=[0]*76
    for pair,(bw,rs) in enumerate(((0,0),(1,0),(0,1),(0,2))):
        for bf in range(2):
            key=(bw,nt,rs,bf,0,0)
            # Missing BF combinations retain source kzalloc zero, not copied non-BF values.
            page[pair*2+bf]=min(lmt.get(key,0)//2,63) # noSAR=63, 2G TPE=63
    for i in range(0,76,4):write(addr,pack(page[i:i+4]));addr+=4
addr=0x12048
for nt in range(2):
    page=[0]*80
    for r in range(5):
        key=(r,nt,0,0)
        page[r*16]=min(ru.get(key,0)//2,63)
    for i in range(0,80,4):write(addr,pack(page[i:i+4]));addr+=4
# Source CV1 antenna difference is zero when regd_enabled=0; references=0.
for p in range(2):
    write(0x2e628+p*0x100,0,0x1ff);write(0x2e628+p*0x100,0,0x3fe00)
    write(0x2e6a0+p*0x100,18<<20,0x3ff00000)
write(0x11a20,0,0x3fe);write(0x11a20,0,0x7fc00)
# Unconfigured SAR query returns 63 MAC units, converted to RF:126.
for p in range(2):write(0x2e61c+p*0x100,126<<12,0x1ff000)
with (ROOT/'artifacts/channel_payload.hpp').open('a') as out:
    out.write('inline const std::vector<archer::Channel1BbWrite> trusted_channel1_world_power{\n')
    for a,m,v in ops:out.write(f'{{{a},{m}u,{v}u}},\n')
    out.write('};\n')
print('Bound world/unconfigured diagnostic power writes:',len(ops))
