"""Offline RTL8922A BE v2 TX body encoder. No USB submission or TX readiness claim."""
import argparse
import json
from pathlib import Path
import struct

# Linux c03c1a14507dfaf8b856c2608f9ac216021fc222, txrx.h/core.c.
# Only the unencrypted body without WDINFO/address-info/WP metadata is supported.
FIELDS = {
    'stf_mode': (0,10,1), 'hdr_llc_len': (0,11,5), 'ch_dma': (0,16,4),
    'pkt_size': (2,0,14), 'agg_en': (2,14,1), 'bk': (2,15,1),
    'qsel': (2,17,6), 'tid_indicate': (2,23,1), 'mac_id': (2,24,8),
    'seq': (3,0,12), 'mlo': (3,12,1), 'sw_mld': (3,13,1),
    'upd_wlan_hdr': (6,13,1), 'er_cap': (7,10,1),
    'gi_ltf': (7,13,3), 'data_rate': (7,16,12), 'data_bw': (7,28,3),
    'use_rate': (7,31,1),
}


def encode_body(fields):
    unknown = set(fields) - set(FIELDS)
    if unknown:
        raise ValueError(f'Unsupported fields: {sorted(unknown)}')
    if not {'pkt_size', 'ch_dma'} <= set(fields):
        raise ValueError('Explicit pkt_size and ch_dma required')
    words = [0] * 8
    values = {'stf_mode': 1, **fields}
    for name, value in values.items():
        word, shift, width = FIELDS[name]
        if type(value) is not int or not 0 <= value < (1 << width):
            raise ValueError(f'{name}: unsigned {width}-bit integer required')
        words[word] |= value << shift
    if fields['pkt_size'] == 0:
        raise ValueError('Empty frame is unsupported')
    if fields['ch_dma'] == 12:
        raise ValueError('Firmware commands use a separate descriptor path')
    return struct.pack('<8I', *words)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('fields_json', type=Path)
    args = parser.parse_args()
    fields = json.loads(args.fields_json.read_text())
    body = encode_body(fields)
    print(json.dumps({'descriptor_format':'BE_v2_body_only', 'body_bytes':len(body),
                      'body_hex':body.hex(), 'words':[f'{w:08x}' for w in struct.unpack('<8I', body)],
                      'hardware_validated':False, 'transmission_ready':False,
                      'limitations':['No WDINFO, encryption, checksum or USB aggregation',
                                     'Queue/rate/port semantics and TX reports require validation']}))
