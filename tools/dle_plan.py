"""Offline RTL8922A USB2 SCC DLE candidate. No USB access or execution support."""
import json

FIFO_BYTES = 589824
WDE = dict(page_bytes=64, linked=508, unlinked=4, offset=0)
PLE = dict(page_bytes=256, linked=2059, unlinked=109, offset=32768)
PLE_NAMES = ('cma0_tx', 'cma1_tx', 'c2h', 'h2c', 'wcpu', 'mpdu_proc',
             'cma0_dma', 'cma1_dma', 'bb_rpt', 'wd_rel', 'cpu_io', 'tx_rpt', 'h2d')
PLE_MIN = (463, 0, 16, 24, 7, 14, 57, 0, 12, 9, 1, 4, 0)
PLE_MAX = (463, 0, 16, 24, 1459, 1466, 1509, 0, 1464, 9, 1, 1456, 0)


def packet_buffer(page_bytes, linked, offset, engine):
    choices = {'wde': {64: 0, 128: 1}, 'ple': {128: 1, 256: 2}}
    if (engine not in choices or page_bytes not in choices[engine]
            or not 0 <= linked <= 0x1fff or offset < 0
            or offset % 8192 or offset // 8192 > 0x7f):
        raise ValueError('Unsupported or unrepresentable DLE layout')
    return (linked << 16) | ((offset // 8192) << 8) | choices[engine][page_bytes]


def quota(minimum, maximum):
    if not 0 <= minimum <= maximum <= 0xfff:
        raise ValueError('Invalid DLE quota')
    return minimum | (maximum << 16)


def candidate():
    total = sum(x['page_bytes'] * (x['linked'] + x['unlinked']) for x in (WDE, PLE)) + 2048
    if total != FIFO_BYTES or WDE['page_bytes'] * (WDE['linked'] + WDE['unlinked']) != PLE['offset']:
        raise ValueError('DLE regions do not fit the FIFO')
    entries = [dict(address=0x8c08, mask=0x1fff7f03,
                    value=packet_buffer(64, 508, 0, 'wde')),
               dict(address=0x9008, mask=0x1fff7f03,
                    value=packet_buffer(256, 2059, 32768, 'ple'))]
    for i, (name, value) in enumerate(zip(('hif', 'wcpu', 'dcpu', 'pkt_in', 'cpu_io'), (492, 6, 0, 0, 10))):
        entries.append(dict(address=0x8c40 + 4*i, name=name, value=quota(value, value)))
    for i, (name, low, high) in enumerate(zip(PLE_NAMES, PLE_MIN, PLE_MAX)):
        entries.append(dict(address=0x9040 + 4*i, name=name, value=quota(low, high)))
    if sum(PLE_MIN) > PLE['linked']:
        raise ValueError('Guaranteed quotas exceed linked pages')
    return dict(chip='RTL8922A', transport='USB2', mode='SCC', executable=False,
                fifo_bytes=total, reserved_tail_bytes=2048,
                wde=WDE, ple=PLE, register_candidates=entries,
                prerequisites=['quiesce firmware/queues before DLE repartition',
                               'validate firmware mode transition',
                               'disable DLE, enable clocks, configure layout and quotas',
                               'enable DLE and poll WDE/PLE readiness',
                               'initialize USB HCI flow control before traffic (PCIe preload is skipped)'],
                limitation='DLE candidate only; not a complete MAC or H2C initialization sequence')


if __name__ == '__main__':
    print(json.dumps(candidate(), indent=2))
