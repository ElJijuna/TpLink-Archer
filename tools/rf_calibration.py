"""Decode raw board/calibration fields from RTL8922A's 0x240-byte RF map.
Offsets follow packed rtw8922a_efuse and core.h TSSI group sizes 6/5/14/32.
Values are raw factory codes, not validated RF programming instructions.
"""
import json
import sys
from pathlib import Path


def decode(data):
    if len(data) != 0x240:
        raise ValueError('Expected 0x240-byte RTL8922A logical RF block')
    paths = []
    for index, start in enumerate((0x10, 0x3a)):
        gain_start = 0xd4 + index * 5
        paths.append(dict(path=index, thermal_raw=data[0xd0+index],
                          tssi_cck_raw=list(data[start:start+6]),
                          tssi_2g_raw=list(data[start+6:start+11]),
                          tssi_5g_raw=list(data[start+18:start+32]),
                          tssi_6g_raw=list(data[0x100+index*42:0x120+index*42]),
                          rx_gain_raw=list(data[gain_start:gain_start+5]),
                          rx_gain_6g_raw=list(data[0x1a8+index*8:0x1b0+index*8])))
    return dict(country_code_raw=list(data[:2]), channel_plan_raw=data[0xb8],
                crystal_trim_raw=data[0xb9], board_info_raw=data[0xc1],
                rfe_type_raw=data[0xca], paths=paths,
                factory_map_populated=any(x != 255 for x in data),
                radio_configuration_applied=False)


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('Usage: rf_calibration.py RF_LOGICAL.bin')
    print(json.dumps(decode(Path(sys.argv[1]).read_bytes()), indent=2))
