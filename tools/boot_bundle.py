"""Generate a compile-bound bundle for the single hash-verified research firmware."""
import hashlib
import json
from pathlib import Path
from firmware_download import prepare, packets
from phy_table import plan

ROOT=Path(__file__).resolve().parents[1]
data=(ROOT/'artifacts/rtw8922a_fw-1.bin').read_bytes()
if hashlib.sha256(data).hexdigest()!='4340bc322912f30f1793bb57a1747c23b15e40db2df628d6687d1ddf722f2828':
    raise SystemExit('Firmware hash mismatch')
report=prepare(data,1,bytes.fromhex('ffffffff'))
lines=['#pragma once','#include "archer/firmware_boot.hpp"',
       'inline std::vector<archer::BootImage> trusted_boot_images(){return {']
for image in report['images']:
    start=image['container_offset']
    # Exact image extent is the sum of parsed header and stored sections.
    size=next(x['file_offset']+x['stored_bytes'] for x in reversed(image['sections']))
    frames=list(packets(data[start:start+size]))
    lines.append('archer::BootImage{{')
    for _,_,frame in frames:lines.append('{'+','.join(str(x) for x in frame)+'},')
    lines.append('}},')
lines.append('};}')
bb=next(x for x in plan(data,0x6933,1,1)['tables'] if x['name']=='BB_REG')
if any(x['kind']!='bb_write' or x['address']%4 or x['address']>0xffff for x in bb['operations']):raise SystemExit('Unhandled BB opcode/address')
lines.append('inline const std::vector<std::pair<unsigned,unsigned>> trusted_bb_table{')
for x in bb['operations']:lines.append('{'+str(0x20000+x['address'])+','+str(x['value'])+'},')
lines.append('};')
(ROOT/'artifacts/bb_payload.hpp').write_text('#pragma once\n#include <vector>\n#include <utility>\n'+'\n'.join(lines[lines.index('inline const std::vector<std::pair<unsigned,unsigned>> trusted_bb_table{'):])+'\n')
(ROOT/'artifacts/boot_payload.hpp').write_text('\n'.join(lines)+'\n')
print(json.dumps({'images':[{'name':i['name'],'packets':i['packet_count']} for i in report['images']],
                  'bb_writes':len(bb['operations'])}))
