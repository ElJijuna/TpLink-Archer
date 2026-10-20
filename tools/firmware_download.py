"""Offline RTL8922A main+BBMCU download packet preparation. No USB executor."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from bbmcu import inventory
from firmware_elements import inspect
from firmware import inspect_v1


def usb_frame(payload, packet_type):
    if packet_type not in (13,14):
        raise ValueError('Expected H2C or firmware-data packet type')
    if not payload or len(payload) > 0x3fff:
        raise ValueError('Invalid firmware packet length')
    padding = bytes(4) if (24 + len(payload)) % 512 == 0 else b''
    if len(payload) + len(padding) > 0x3fff:
        raise ValueError('Firmware packet exceeds descriptor width')
    return struct.pack('<I', (packet_type << 24) | (len(payload) + len(padding))) + bytes(20) + payload + padding


def packets(image, sequence=0):
    if type(sequence) is not int or not 0 <= sequence <= 255:
        raise ValueError('Invalid fresh-boot header sequence')
    info = inspect_v1(image)
    # Input parser rejects MSSC arrays. No key/signature substitution is implemented.
    header_length = info['header_bytes'] - info['dynamic_header_bytes']
    header = image[:header_length]
    h2c = struct.pack('<II', 1 | (3 << 2) | (sequence << 24), 8 + len(header))
    yield 'header', None, usb_frame(h2c + header, 13)
    for section in info['sections']:
        start = section['file_offset']
        for offset in range(0, section['stored_bytes'], info['part_size']):
            payload = image[start + offset:start + min(offset + info['part_size'], section['stored_bytes'])]
            yield 'section', section['index'], usb_frame(payload, 14)


def prepare(data, cv, security):
    if security != bytes.fromhex('ffffffff'):
        raise ValueError('Unsupported firmware security selection; no download plan')
    selected = inventory(data, cv)
    elements = inspect(data)
    main = [x for x in elements['images'] if x['cut_code'] == cv and x['type'] == 1 and x['mp'] == 0]
    bb = [x for x in selected['bbmcu_candidates'] if x['selected']]
    if len(main) != 1 or len(bb) != 1 or bb[0]['element_id'] != 0:
        raise ValueError('Expected one normal image and one BBMCU0 candidate')
    main_entry = main[0]
    bb_entry = next(e for e in elements['elements'] if e['header_offset'] == bb[0]['header_offset'])
    images = [('normal', main_entry['offset'], main_entry['bytes']),
              ('bbmcu0', bb_entry['payload_offset'], bb_entry['bytes'])]
    report = []
    for name, start, size in images:
        image = data[start:start + size]
        info = inspect_v1(image)
        frames = []
        for index, (kind, section, packet) in enumerate(packets(image)):
            frames.append(dict(index=index, kind=kind, section=section,
                               usb_bytes=len(packet), sha256=hashlib.sha256(packet).hexdigest()))
        report.append(dict(name=name, container_offset=start, image_sha256=info['sha256'],
                           version=info['version'], part_size=info['part_size'],
                           header_bytes_sent=info['header_bytes']-info['dynamic_header_bytes'],
                           sections=info['sections'], packet_count=len(frames), packets=frames))
    return dict(container_sha256=selected['container_sha256'], cv=cv, images=report,
                security_selector='0xffff (default path in pinned Linux decoder)',
                executable=False, hardware_writes_performed=False,
                required_order=['verify target/power/identity and exclusive ownership',
                                'prepare MAC transport and BBMCU before firmware reset',
                                'hold/reset CPU and enable WLAN+BBMCU0 download',
                                'poll H2C ready; send normal header; poll download path ready',
                                'send normal sections; poll WLAN download-enable clear',
                                'poll H2C ready; send BBMCU0 header; poll download path ready',
                                'send BBMCU0 sections; poll BBMCU0 download-enable clear',
                                'wait final firmware initialization and establish new sequence lifecycle',
                                'initialize BB tables/gain/postinit, then RF'],
                limitations=['No native USB upload, CPU reset or MAC/BBMCU preinit executor',
                             'Header sequence 0 is a cold-start candidate only, never a live H2C reservation',
                             'Default efuse selector does not cryptographically verify firmware',
                             'Device ROM acceptance, pairing and recovery remain untested'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('firmware', type=Path)
    parser.add_argument('cv', type=lambda s: int(s, 0))
    parser.add_argument('security', type=Path)
    args = parser.parse_args()
    try:
        print(json.dumps(prepare(args.firmware.read_bytes(), args.cv, args.security.read_bytes()), indent=2))
    except (ValueError, OSError) as e:
        parser.exit(2, f'firmware-download: {e}\n')
