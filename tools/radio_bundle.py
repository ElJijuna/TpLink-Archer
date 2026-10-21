"""Generate compile-bound RTL8922A RFE1/CV1 radio/NCTL tables; no device access."""
import hashlib
from pathlib import Path
from phy_table import plan
ROOT=Path(__file__).resolve().parents[1]
data=(ROOT/'artifacts/rtw8922a_fw-1.bin').read_bytes()
if hashlib.sha256(data).hexdigest()!='4340bc322912f30f1793bb57a1747c23b15e40db2df628d6687d1ddf722f2828':raise SystemExit('Firmware hash mismatch')
r=plan(data,0x6933,1,1)
lines=['#pragma once','#include "archer/radio_init.hpp"','inline const std::vector<archer::RadioTable> trusted_radio_tables{']
for path,name in enumerate(('RADIO_A','RADIO_B')):
 t=next(x for x in r['tables'] if x['name']==name)
 lines.append('archer::RadioTable{'+str(path)+',{')
 for op in t['operations']:
  if op['kind'] not in ('rf_write','delay'):raise SystemExit('Unsupported radio opcode')
  lines.append('{'+','.join(str(x) for x in (op['address'],op['value'],op.get('microseconds',0)))+'},')
 lines.append('}},')
lines.append('};')
lines.append('inline const std::vector<std::pair<unsigned,unsigned>> trusted_nctl_table{')
for op in next(x for x in r['tables'] if x['name']=='RF_NCTL')['operations']:
 if op['kind']!='bb_write' or op['address']%4 or op['address']>0xffff:raise SystemExit('Unsupported NCTL opcode/address')
 lines.append('{'+str(op['address'])+','+str(op['value'])+'},')
lines.append('};')
(ROOT/'artifacts/radio_payload.hpp').write_text('\n'.join(lines)+'\n')
print('Bound radio A/B and NCTL tables generated')
