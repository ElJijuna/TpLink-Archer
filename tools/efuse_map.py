"""Offline RTL8922A DDV physical-to-logical decoding; never programs efuse."""
from pathlib import Path
import argparse


def decode_block(physical, page, offset, size):
    if len(physical) != 0x1300:
        raise ValueError('Expected complete RTL8922A DDV map (0x1300 bytes)')
    if not 0 <= page <= 6 or not 0 <= offset <= 0xffff or size < 1 or offset + size > 0x10000:
        raise ValueError('Invalid DDV logical range; DAV page is unsupported')
    output = bytearray(b'\xff' * size)
    cursor = 4  # RTL8922A security-control prefix.
    while cursor < len(physical):
        if len(physical) - cursor < 3:
            raise ValueError('Truncated DDV header')
        header_bytes = physical[cursor:cursor+3]
        if 0xff in header_bytes:
            break  # Same invalid-header termination rule as Linux BE parser.
        header = int.from_bytes(header_bytes, 'big')
        cursor += 3
        entry_page = (header >> 17) & 7
        block = (header >> 4) & 0x1fff
        enabled_mask = header & 15
        for word in range(4):
            if enabled_mask & (1 << word):
                continue
            if cursor + 2 > len(physical):
                raise ValueError('Truncated DDV word')
            logical = block * 8 + word * 2
            if entry_page == page:
                for byte in range(2):
                    target = logical + byte - offset
                    if 0 <= target < size:
                        output[target] = physical[cursor + byte]
            cursor += 2
    return bytes(output)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('physical_map')
    parser.add_argument('new_rf_map')
    args = parser.parse_args()
    decoded = decode_block(Path(args.physical_map).read_bytes(), 1, 0, 0x240)
    with open(args.new_rf_map, 'xb') as stream:
        stream.write(decoded)
