"""Bind only the existing hash-verified firmware into immutable DriverKit arrays."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from firmware_download import prepare,packets
SOURCE_SHA256='4340bc322912f30f1793bb57a1747c23b15e40db2df628d6687d1ddf722f2828'

def bound_frames(data):
    if hashlib.sha256(data).hexdigest()!=SOURCE_SHA256:
        raise ValueError('Firmware hash mismatch; no generated files changed')
    report=prepare(data,1,bytes.fromhex('ffffffff'))
    result=[]
    for image in report['images']:
        start=image['container_offset']
        extent=next(s['file_offset']+s['stored_bytes'] for s in reversed(image['sections']))
        frames=[frame for _,_,frame in packets(data[start:start+extent])]
        if not 2<=len(frames)<=512:raise ValueError('Unsupported image packet count')
        for i,frame in enumerate(frames):
            word=struct.unpack_from('<I',frame)[0]
            if not 25<=len(frame)<=16384 or word&0x3fff!=len(frame)-24 or (word>>24)&63!=(14 if i else 13):
                raise ValueError('Unexpected firmware framing')
        result.append((image['name'],frames))
    if len(result)!=2 or sum(len(f) for _,frames in result for f in frames)>8*1024*1024:
        raise ValueError('Unsupported image bundle')
    return result

def generate(data,output):
    images=bound_frames(data)  # Authenticate source BEFORE creating/writing output.
    lines=['#pragma once','#include "archer/driver_firmware_engine.hpp"',f'inline constexpr char driver_firmware_source_sha256[]="{SOURCE_SHA256}";']
    manifest={'source_sha256':SOURCE_SHA256,'profile':'RTL8922A CV1, BBMCU0, original security ffffffff','images':[]}
    for i,(name,frames) in enumerate(images):
        for j,frame in enumerate(frames):
            lines.append(f'inline constexpr std::uint8_t driver_fw_{i}_{j}[]={{'+','.join(str(x) for x in frame)+'};')
        lines.append(f'inline constexpr archer::DriverFirmwareFrame driver_frames_{i}[]={{')
        for j,frame in enumerate(frames):lines.append(f'{{driver_fw_{i}_{j},{len(frame)}}},')
        lines.append('};')
        manifest['images'].append({'name':name,'packets':len(frames),'bytes':sum(map(len,frames)),'frame_sha256':[hashlib.sha256(f).hexdigest() for f in frames]})
    lines.append('inline constexpr std::array<archer::DriverFirmwareImage,2> driver_bound_firmware{{')
    for i,(_,frames) in enumerate(images):lines.append(f'{{driver_frames_{i},{len(frames)}}},')
    lines.append('}};')
    output=Path(output);output.mkdir(parents=True,exist_ok=True)
    (output/'BoundFirmware.hpp').write_text('\n'.join(lines)+'\n')
    (output/'firmware-plan.json').write_text(json.dumps(manifest,indent=2)+'\n')
    return manifest

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,default=ROOT/'artifacts/rtw8922a_fw-1.bin')
    parser.add_argument('--output',type=Path,default=ROOT/'build/driverkit/firmware')
    args=parser.parse_args()
    result=generate(args.input.read_bytes(),args.output)
    print(json.dumps({'images':len(result['images']),'packets':sum(i['packets'] for i in result['images']),'source_sha256':result['source_sha256']}))
