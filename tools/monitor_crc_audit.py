"""Offline CRC32 audit for this experiment's classic radiotap PCAP profile."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib


def audit(data):
    if len(data)<24 or data[:4]!=b'\xd4\xc3\xb2\xa1' or struct.unpack_from('<I',data,20)[0]!=127:
        raise ValueError('Expected little-endian classic PCAP/radiotap')
    result=dict(packets=0,crc_valid=0,crc_invalid=0,truncated_packets=0,
        fcs_unavailable=0,flag_crc_disagreements=0,valid_target_beacons=0,
        valid_ack=0,valid_cts=0,valid_test_mac_frames=0,frequencies_mhz={})
    cursor=24;test=bytes.fromhex('0200008922a1');ap=bytes.fromhex('50926a110666')
    while cursor<len(data):
        if len(data)-cursor<16:raise ValueError('Incomplete PCAP record header')
        _,_,length,original=struct.unpack_from('<IIII',data,cursor);cursor+=16
        if length>original or length>len(data)-cursor:raise ValueError('Invalid PCAP record extent')
        packet=data[cursor:cursor+length];cursor+=length;result['packets']+=1
        if len(packet)<8 or packet[0]!=0:raise ValueError('Invalid radiotap header')
        header=struct.unpack_from('<H',packet,2)[0]
        if not 8<=header<=len(packet):raise ValueError('Invalid radiotap extent')
        offset=4;first=None
        while True:
            if offset+4>header:raise ValueError('Truncated radiotap presence words')
            word=struct.unpack_from('<I',packet,offset)[0];offset+=4
            if first is None:first=word
            if not word&0x80000000:break
        flags=None;frequency=None
        for bit,alignment,size in ((0,8,8),(1,1,1),(2,1,1),(3,2,4)):
            if not first&(1<<bit):continue
            offset=(offset+alignment-1)//alignment*alignment
            if offset+size>header:raise ValueError('Radiotap field outside header')
            if bit==1:flags=packet[offset]
            if bit==3:frequency=struct.unpack_from('<H',packet,offset)[0]
            offset+=size
        key=str(frequency) if frequency is not None else 'unknown'
        result['frequencies_mhz'][key]=result['frequencies_mhz'].get(key,0)+1
        if length!=original:result['truncated_packets']+=1;continue
        if flags is None or not flags&0x10 or flags&0x20:
            result['fcs_unavailable']+=1;continue # Padded frame format is outside this profile.
        frame=packet[header:]
        if len(frame)<14:raise ValueError('Frame with FCS shorter than minimum ACK')
        valid=zlib.crc32(frame[:-4])==int.from_bytes(frame[-4:],'little')
        result['crc_valid' if valid else 'crc_invalid']+=1
        if valid==bool(flags&0x40):result['flag_crc_disagreements']+=1
        if not valid:continue
        frame=frame[:-4];kind=frame[0]&0xfc
        if kind==0xd4:result['valid_ack']+=1
        if kind==0xc4:result['valid_cts']+=1
        addresses=[frame[4:10]]
        if frame[0]&0x0c==0 and len(frame)>=24:addresses += [frame[10:16],frame[16:22]]
        elif kind==0xb4 and len(frame)>=16:addresses.append(frame[10:16])
        if test in addresses:result['valid_test_mac_frames']+=1
        if kind==0x80 and len(frame)>=36 and frame[16:22]==ap:
            ie=36
            while ie+2<=len(frame):
                ident,size=frame[ie:ie+2];ie+=2
                if ie+size>len(frame):break
                if ident==0 and frame[ie:ie+size]==b'Especial':result['valid_target_beacons']+=1;break
                ie+=size
    result.update(source_sha256=hashlib.sha256(data).hexdigest(),antenna_emission_proven=False)
    return result


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('pcap',type=Path)
    print(json.dumps(audit(parser.parse_args().pcap.read_bytes()),indent=2))
