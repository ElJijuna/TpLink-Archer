"""Offline CV selection of RTL8922A main/BBMCU images. Never uploads firmware."""
import argparse
import json
from pathlib import Path
from firmware_elements import inspect
from firmware import inspect_v1


def inventory(data, cv):
    if type(cv) is not int or not 0 <= cv <= 255:
        raise ValueError('Invalid digital revision')
    container = inspect(data)
    candidates, selected = [], {}
    for element in container['elements']:
        if element['id'] not in (0, 1):
            continue
        start, size = element['payload_offset'], element['bytes']
        parsed = inspect_v1(data[start:start + size])
        cut = data[element['header_offset'] + 24]
        candidate = dict(element_id=element['id'], name=element['name'],
                         header_offset=element['header_offset'], cv_minimum=cut,
                         aid_metadata=element['aid'], image=parsed,
                         selected=False)
        # Linux BBMCU callback takes the FIRST suitable CV per MCU in file order.
        # Unlike register-table callbacks, this callback does not filter AID.
        if cut <= cv and element['id'] not in selected:
            candidate['selected'] = True
            selected[element['id']] = candidate
        candidates.append(candidate)
    main = [x for x in container['images'] if x['cut_code'] == cv and x['type'] == 1 and x['mp'] == 0]
    if len(main) > 1:
        raise ValueError('Ambiguous normal firmware image')
    main_info = None
    if main:
        entry = main[0]
        main_info = inspect_v1(data[entry['offset']:entry['offset'] + entry['bytes']])
    return dict(container_sha256=container['sha256'], cv=cv,
                normal_firmware_candidate=main_info, bbmcu_candidates=candidates,
                selected_mcus=sorted(selected), resident_firmware_identity=None,
                hardware_compatibility_verified=False, hardware_writes_performed=False,
                limitations=['File selection is not proof of resident firmware identity or BBMCU execution',
                             'Signatures and checksums have not been verified',
                             'Source and firmware commits are independently pinned, not a verified pair'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('path', type=Path)
    parser.add_argument('cv', type=lambda s: int(s, 0))
    args = parser.parse_args()
    try:
        print(json.dumps(inventory(args.path.read_bytes(), args.cv), indent=2))
    except (ValueError, OSError) as e:
        parser.exit(2, f'bbmcu: {e}\n')
