"""Offline RTL8922A BE v2 TX descriptor encoder. No USB submission or TX readiness claim."""
import argparse
import json
from pathlib import Path
import struct

# Linux c03c1a14507dfaf8b856c2608f9ac216021fc222, txrx.h/core.c.
# Unencrypted descriptors only; no address-info/WP metadata.
FIELDS = {
    'stf_mode': (0,10,1), 'hdr_llc_len': (0,11,5), 'ch_dma': (0,16,4),
    'pkt_size': (2,0,14), 'agg_en': (2,14,1), 'bk': (2,15,1),
    'qsel': (2,17,6), 'tid_indicate': (2,23,1), 'mac_id': (2,24,8),
    'seq': (3,0,12), 'mlo': (3,12,1), 'sw_mld': (3,13,1),
    'upd_wlan_hdr': (6,13,1), 'er_cap': (7,10,1),
    'gi_ltf': (7,13,3), 'data_rate': (7,16,12), 'data_bw': (7,28,3),
    'use_rate': (7,31,1),
}


INFO_FIELDS = {
    'port': (0,4,3), 'dis_data_fb': (0,10,1), 'ldpc': (0,11,1),
    'stbc': (0,12,1), 'tx_cnt_lmt': (0,16,6), 'tx_cnt_lmt_en': (0,22,1),
    'ampdu_num': (1,0,8), 'a_ctrl_bsr': (1,14,1),
    'data_retry_lowest_rate': (1,16,12), 'sn': (1,28,4),
    'ampdu_density': (2,18,3), 'report': (2,30,1),
}


def encode_info(fields):
    unknown = set(fields) - (set(INFO_FIELDS) | {'is_bmc'})
    if unknown:
        raise ValueError(f'Unsupported WDINFO fields: {sorted(unknown)}')
    if type(fields.get('is_bmc')) is not int or fields['is_bmc'] not in (0,1):
        raise ValueError('Explicit is_bmc integer 0 or 1 required')
    words = [0] * 8
    for name, value in fields.items():
        if name == 'is_bmc':
            continue
        word, shift, width = INFO_FIELDS[name]
        if type(value) is not int or not 0 <= value < (1 << width):
            raise ValueError(f'{name}: unsigned {width}-bit integer required')
        words[word] |= value << shift
    # core.c rtw89_build_txwd_info4_v2: RTS_EN=!is_bmc, HW_RTS_EN=1.
    words[4] = (1 << 31) | ((1-fields['is_bmc']) << 27)
    return struct.pack('<8I', *words)


def encode_descriptor(fields, info=None):
    body = encode_body(fields)
    if info is None:
        return body
    extension = encode_info(info)
    words = list(struct.unpack('<8I', body))
    words[0] |= 1 << 22  # BE_TXD_BODY0_WDINFO_EN, paired with actual extension.
    return struct.pack('<8I', *words) + extension


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


# rtw8922au.c bulkout_id indexes the OUT endpoints in descriptor order.
BULKOUT_INDEX = {0:3, 2:5, 4:4, 6:6, 8:0, 9:0, 10:1, 11:1}


def usb2_out_endpoints(configuration):
    if len(configuration) < 9 or configuration[:2] != bytes([9,2]):
        raise ValueError('Invalid USB configuration header')
    if int.from_bytes(configuration[2:4], 'little') != len(configuration):
        raise ValueError('USB configuration length mismatch')
    outs, ins, interfaces = [], [], []
    offset = 0
    while offset < len(configuration):
        size = configuration[offset]
        if size < 2 or offset+size > len(configuration):
            raise ValueError('Truncated USB descriptor')
        d = configuration[offset:offset+size]
        if d[1] == 4:
            if size != 9:
                raise ValueError('Invalid USB interface')
            interfaces.append(d)
        if d[1] == 5:
            if not interfaces or size != 7 or d[3] != 2 or int.from_bytes(d[4:6],'little') != 512:
                raise ValueError('Expected USB2 bulk endpoints of512 bytes')
            (ins if d[2]&128 else outs).append(d[2])
        offset += size
    if len(interfaces) != 1 or interfaces[0][2:6] != bytes([0,0,9,255]):
        raise ValueError('Expected single vendor interface0 alternate0 with9 endpoints')
    if outs != [5,6,7,9,10,11,12] or ins != [132,136]:
        raise ValueError('Unexpected Archer USB2 endpoint order')
    return outs


def usb_packet_plan(fields, frame, configuration, info=None):
    descriptor = encode_descriptor(fields, info)
    if len(frame) != fields['pkt_size']:
        raise ValueError('Descriptor packet size differs from frame length')
    if fields.get('stf_mode',1) != 1:
        raise ValueError('USB network path requires STF_MODE1')
    if fields['ch_dma'] not in BULKOUT_INDEX:
        raise ValueError('No explicitly mapped DMA channel for this chip')
    outs = usb2_out_endpoints(configuration)
    packet = descriptor + frame
    return {'endpoint_address': outs[BULKOUT_INDEX[fields['ch_dma']]],
            'dma_channel':fields['ch_dma'], 'descriptor_bytes':len(descriptor),
            'frame_bytes':len(frame), 'transfer_bytes':len(packet),
            'transfer_hex':packet.hex(), 'padding_bytes':0,
            'zero_packet_required':len(packet)%512==0,
            'hardware_validated':False, 'transmission_ready':False}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('fields_json', type=Path)
    parser.add_argument('--wdinfo-json', type=Path, help='Optional unencrypted WDINFO fields; explicit is_bmc required')
    parser.add_argument('--frame-bin', type=Path, help='Offline frame bytes for USB packet plan')
    parser.add_argument('--usb-config', type=Path, help='Recorded USB2 configuration descriptor')
    args = parser.parse_args()
    if bool(args.frame_bin) != bool(args.usb_config):
        parser.error('--frame-bin and --usb-config must be provided together')
    fields = json.loads(args.fields_json.read_text())
    info = json.loads(args.wdinfo_json.read_text()) if args.wdinfo_json else None
    body = encode_descriptor(fields, info)
    if args.frame_bin:
        print(json.dumps(usb_packet_plan(fields, args.frame_bin.read_bytes(), args.usb_config.read_bytes(), info)))
        raise SystemExit(0)
    print(json.dumps({'descriptor_format':'BE_v2_with_wdinfo' if info is not None else 'BE_v2_body_only', 'descriptor_bytes':len(body),
                      'body_bytes':32, 'wdinfo_bytes':len(body)-32, 'body_hex':body[:32].hex(),
                      'descriptor_hex':body.hex(), 'words':[f'{w:08x}' for w in struct.unpack('<' + 'I'*(len(body)//4), body)],
                      'hardware_validated':False, 'transmission_ready':False,
                      'limitations':['No encryption, checksum or USB aggregation',
                                     'Queue/rate/port semantics and TX reports require validation']}))
