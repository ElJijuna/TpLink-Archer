"""Bind physical factory RF data and pinned firmware tracking for TX RFK."""
from pathlib import Path
import hashlib
ROOT=Path(__file__).resolve().parents[1]
fw=(ROOT/'artifacts/rtw8922a_fw-1.bin').read_bytes()
rf=(ROOT/'artifacts/efuse-stage13/rf-logical.bin').read_bytes()
if hashlib.sha256(fw).hexdigest()!='4340bc322912f30f1793bb57a1747c23b15e40db2df628d6687d1ddf722f2828':
    raise SystemExit('Firmware identity mismatch')
if hashlib.sha256(rf).hexdigest()!='f105aa4a4efd345b6533da26d6dfb250bda461533a8ca962e27ad3083600fc37' or len(rf)!=0x240 or rf[0xca]!=1:
    raise SystemExit('Factory RF board profile mismatch')
tracking=fw[1580656:1580656+1112]
if hashlib.sha256(tracking[32:]).hexdigest()!='f95a86676fd251e503f4c89c77af2152bedb95525de28c1fef701e26918d15ce':
    raise SystemExit('TX power tracking identity mismatch')
lines=['#pragma once','#include "archer/tx_rfk.hpp"']
for name,data,decoder,args in [('trusted_tssi_board',rf,'decode_tssi_board',''),
                               ('trusted_tssi_tracking',tracking,'decode_tssi_tracking_element',',0x6933')]:
    lines.append('inline const auto '+name+'=archer::'+decoder+'(std::vector<std::uint8_t>{'+','.join(map(str,data))+'}'+args+');')
(ROOT/'artifacts/tx_calibration_payload.hpp').write_text('\n'.join(lines)+'\n')
print('Bound factory RF map and firmware thermal tracking generated')
