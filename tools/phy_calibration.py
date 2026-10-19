"""Offline RTL8922A PHY trim decoding from the 0x1700..0x1737 efuse region."""
import json
from pathlib import Path
import sys


def decode(data):
    if len(data) != 0x38:
        raise ValueError('Expected 0x38-byte RTL8922A PHY trim region')
    thermal_raw = [data[6], data[0x33]]
    thermal = [0, 0]
    thermal_programmed = True
    for i, raw in enumerate(thermal_raw):
        if raw == 255:
            thermal_programmed = False
            break  # Linux stops at first erased path; global flag gates use.
        magnitude = raw & 63
        thermal[i] = magnitude if raw & 64 else -magnitude
    bias_programmed = data[0] != 255
    paths = []
    for i, (pa_offset, pad_offset) in enumerate(((7, 8), (0x34, 0x35))):
        pa, pad = data[pa_offset], data[pad_offset]
        paths.append(dict(path=i, thermal_raw=thermal_raw[i], thermal_trim=thermal[i],
                          pa_raw=pa, pa_2g=pa & 15, pa_5g=pa >> 4,
                          pad_raw=pad, pad_2g=pad & 15, pad_5g=pad >> 4))
    return dict(thermal_programmed=thermal_programmed,
                pa_and_pad_programmed=bias_programmed, paths=paths,
                rf_writes_applied=False)


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('Usage: phy_calibration.py PHY_CAP.bin')
    print(json.dumps(decode(Path(sys.argv[1]).read_bytes()), indent=2))
