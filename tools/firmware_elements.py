"""Bounded offline inventory of Linux RTL8922A MFW and appended PHY elements."""
import hashlib
import json
import struct
import sys
from pathlib import Path

REG2_IDS = {2, 3, 4, 5, 6, 7, 8}
POWER_IDS = {9, 10, 11, 12, 13, 14, 15, 16, 17, 21, 22, 23, 24, 25, 26}
NAMES = {0:'BBMCU0',1:'BBMCU1',2:'BB_REG',3:'BB_GAIN',4:'RADIO_A',5:'RADIO_B',
         8:'RF_NCTL',9:'TXPWR_BYRATE',10:'TXPWR_LMT_2G',11:'TXPWR_LMT_5G',
         12:'TXPWR_LMT_6G',18:'TXPWR_TRK',19:'RFKLOG_FMT',20:'REGD'}


def inspect(data, rfe=1, aid=None):
    if len(data)<16 or data[0]!=255 or not data[1]:
        raise ValueError('Expected MFW container')
    if not isinstance(rfe, int) or not 0<=rfe<=255:raise ValueError('Invalid RFE type')
    chip_aid=aid
    if chip_aid is not None and not 0<=chip_aid<=65535:raise ValueError('Invalid chip AID')
    end_table=16+16*data[1]
    if end_table>len(data):raise ValueError('Truncated MFW directory')
    images=[]
    previous=end_table
    for index in range(data[1]):
        cv,kind,mp,_,offset,size=struct.unpack_from('<4BII',data,16+16*index)
        if not size or offset<previous or offset+size>len(data):
            raise ValueError('Invalid, overlapping or unordered MFW image')
        images.append(dict(index=index,cut_code=cv,type=kind,mp=mp,offset=offset,bytes=size,
                           sha256=hashlib.sha256(data[offset:offset+size]).hexdigest()))
        previous=offset+size
    cursor=(previous+15)&~15
    elements=[]
    selected_power={}
    selected_registers={}
    while cursor<len(data):
        if len(data)-cursor<32:raise ValueError('Truncated element header')
        ident,size=struct.unpack_from('<II',data,cursor)
        element_aid=struct.unpack_from('<H',data,cursor+12)[0]
        start=cursor+32;end=start+size
        if not size or end>len(data):raise ValueError('Invalid element extent')
        e=dict(id=ident,name=NAMES.get(ident,f'ID_{ident}'),header_offset=cursor,
               payload_offset=start,bytes=size,aid=element_aid,version=list(data[cursor+8:cursor+12]),
               sha256=hashlib.sha256(data[start:end]).hexdigest())
        if ident in REG2_IDS:
            if size%8:raise ValueError('Partial register pair')
            index=data[cursor+24]
            if ident in {4,5,6,7} and index>=4:
                raise ValueError('Radio table index exceeds four slots')
            e.update(register_pairs=size//8,index=index)
            target={2:'bb_tbl',3:'bb_gain',8:'rf_nctl'}.get(ident,f'rf_radio[{index}]')
            e['target']=target
            # Linux accepts AID=0 or the physical AID, then retains first table
            # for each target. Radio slot comes from metadata, not element ID.
            if (element_aid==0 or element_aid==chip_aid) and target not in selected_registers:
                selected_registers[target]=e
        if ident in POWER_IDS:
            entry_rfe,entry_size=data[cursor+26:cursor+28]
            count=struct.unpack_from('<I',data,cursor+28)[0]
            if not entry_size or count*entry_size!=size:
                raise ValueError('Invalid TX power entries')
            e.update(rfe_type=entry_rfe,entry_size=entry_size,entry_count=count)
            # Follow AID filtering and last RFE-match/default semantics independently.
            old=selected_power.get(ident)
            if (e['aid']==0 or e['aid']==chip_aid) and (entry_rfe==rfe or (entry_rfe==0 and (old is None or old['rfe_type']==0))):
                selected_power[ident]=e
        elements.append(e)
        cursor=(end+15)&~15
    return dict(sha256=hashlib.sha256(data).hexdigest(),bytes=len(data),images=images,
                elements=elements,rfe_type=rfe,power_candidates=list(selected_power.values()),
                register_candidates=list(selected_registers.values()),
                aid_selection_scope='generic_only' if chip_aid is None else 'generic_and_matching',
                actual_chip_aid_known=chip_aid is not None,chip_aid=chip_aid,hardware_compatibility_validated=False,
                hardware_writes_performed=False)


if __name__=='__main__':
    if len(sys.argv) not in (2,3):raise SystemExit('Usage: firmware_elements.py MFW.bin [AID]')
    print(json.dumps(inspect(Path(sys.argv[1]).read_bytes(), aid=int(sys.argv[2],0) if len(sys.argv)==3 else None),indent=2))
