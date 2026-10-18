"""Inspect bounded RTL8922A v1 images or extract REA-bound images from one Windows SYS.

Offline only. Structural validation does not verify signatures or hardware compatibility.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

CARRIER_SHA256 = '68f735cc46b25ce983997d7f8693b24ea75af3abaf42a2f4d69fb63ad5590332'
BINDING_EVIDENCE = 'ev_1de042857caf0af0d8ade38446fff4c936e5ded230049fcf107ef9866e0ed510'
# Addresses and size globals referenced by mac_query_fw_buff at VA 0x1403deaa0.
IMAGES = ((0x1405a4870, 0x1405a4860), (0x1405f2100, 0x1405a4864))

def u32(data, offset):
    if offset < 0 or offset + 4 > len(data):
        raise ValueError('Truncated 32-bit field')
    return struct.unpack_from('<I', data, offset)[0]

def inspect_v1(data):
    if len(data) < 48:
        raise ValueError('Truncated v1 header')
    words = struct.unpack_from('<12I', data)
    if words[3] >> 24 != 1 or data[1:4] != b'\x71\x22\x89':
        raise ValueError('Expected observed RTL8922A v1 header')
    count = (words[6] >> 8) & 255
    base = 48 + 16 * count
    if not count or base > len(data):
        raise ValueError('Invalid section table')
    dynamic = bool(words[7] & (1 << 16))
    header_size = words[5] >> 16 if dynamic else base
    if not base <= header_size <= len(data):
        raise ValueError('Invalid firmware header size')
    if dynamic and (header_size - base < 8 or u32(data, base) != header_size - base):
        raise ValueError('Invalid dynamic header length')
    if not words[7] & 0xffff:
        raise ValueError('Zero download part size')
    cursor = header_size
    sections = []
    for index in range(count):
        address, flags, metadata, reserved = struct.unpack_from('<4I', data, 48 + index * 16)
        kind = (flags >> 24) & 15
        size = flags & 0xffffff
        checksum_bytes = 8 if flags & (1 << 28) else 0
        if kind == 9 and metadata & 255:
            raise ValueError('MSSC signature arrays are not supported by this inspector')
        stored_size = size + checksum_bytes
        if not stored_size or cursor + stored_size > len(data):
            raise ValueError('Section exceeds image bounds')
        if address + size > 0x100000000:
            raise ValueError('Section address overflows 32 bits')
        sections.append({'index': index, 'type': kind, 'download_address': address,
                         'file_offset': cursor, 'payload_bytes': size,
                         'stored_bytes': stored_size, 'checksum_bytes': checksum_bytes,
                         'flags': flags, 'metadata': metadata, 'reserved': reserved,
                         'sha256': hashlib.sha256(data[cursor:cursor + stored_size]).hexdigest()})
        cursor += stored_size
    if cursor != len(data):
        raise ValueError('Unaccounted trailing image bytes')
    return {'sha256': hashlib.sha256(data).hexdigest(), 'bytes': len(data),
            'header_version': 1, 'cut_byte': data[0],
            'version': '.'.join(str((words[1] >> shift) & 255) for shift in (0, 8, 16, 24)),
            'commit_id': f'{words[2]:08x}', 'header_bytes': header_size,
            'dynamic_header_bytes': header_size - base,
            'part_size': words[7] & 0xffff, 'sections': sections,
            'validation': 'structure_only', 'signature_verified': False,
            'hardware_compatibility_verified': False}

def pe_file_offset(data, address, size):
    if data[:2] != b'MZ':
        raise ValueError('Not a PE carrier')
    pe = u32(data, 0x3c)
    if pe + 24 > len(data) or data[pe:pe + 4] != b'PE\x00\x00':
        raise ValueError('Invalid PE header')
    count, optional_size = struct.unpack_from('<H12xH', data, pe + 6)
    optional = pe + 24
    if optional + optional_size > len(data) or optional_size < 32:
        raise ValueError('Truncated PE optional header')
    if struct.unpack_from('<H', data, optional)[0] != 0x20b:
        raise ValueError('Expected PE32+')
    image_base = struct.unpack_from('<Q', data, optional + 24)[0]
    rva = address - image_base
    table = optional + optional_size
    if not count or table + count * 40 > len(data):
        raise ValueError('Invalid PE section table')
    matches = []
    for index in range(count):
        offset = table + index * 40
        _, virtual, raw_size, raw_start = struct.unpack_from('<4I', data, offset + 8)
        delta = rva - virtual
        if 0 <= delta and delta + size <= raw_size and raw_start + delta + size <= len(data):
            matches.append(raw_start + delta)
    if len(matches) != 1:
        raise ValueError('Address does not map to exactly one file-backed section')
    return matches[0]

def extract_bound_images(data):
    digest = hashlib.sha256(data).hexdigest()
    if digest != CARRIER_SHA256:
        raise ValueError('Carrier SHA-256 does not match the REA analysis profile')
    images = []
    for address, size_address in IMAGES:
        length = u32(data, pe_file_offset(data, size_address, 4))
        offset = pe_file_offset(data, address, length)
        blob = data[offset:offset + length]
        info = inspect_v1(blob)
        info.update({'carrier_sha256': digest, 'carrier_file_offset': offset,
                     'carrier_virtual_address': f'0x{address:x}',
                     'size_virtual_address': f'0x{size_address:x}',
                     'rea_evidence_id': BINDING_EVIDENCE,
                     'filename': f'rtl8922a-cut{info["cut_byte"]}-{info["version"]}.bin'})
        images.append((info, blob))
    return images

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    inspect = sub.add_parser('inspect')
    inspect.add_argument('path', type=Path)
    extract = sub.add_parser('extract-windows')
    extract.add_argument('path', type=Path)
    extract.add_argument('--output-dir', type=Path)
    args = parser.parse_args()
    try:
        data = args.path.read_bytes()
        if args.command == 'inspect':
            result = inspect_v1(data)
        else:
            images = extract_bound_images(data)
            result = [info for info, _ in images]
            if args.output_dir:
                args.output_dir.mkdir(parents=False, exist_ok=False)
                for info, blob in images:
                    (args.output_dir / info['filename']).write_bytes(blob)
                (args.output_dir / 'manifest.json').write_text(json.dumps(result, indent=2) + '\n')
        print(json.dumps(result, indent=2))
        return 0
    except (OSError, ValueError, struct.error) as error:
        parser.exit(2, f'firmware: {error}\n')

if __name__ == '__main__':
    raise SystemExit(main())
