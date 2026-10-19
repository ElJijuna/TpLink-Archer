"""Summarize successful register observations; never accesses USB or writes registers."""
import json
import sys

REGISTERS = {0x6000: 'rx_aggregation', 0x6004: 'rx_aggregation_1',
             0x7880: 'hci_functions', 0x8400: 'dmac_functions',
             0x8404: 'dmac_clocks', 0x8c08: 'wde_packet_buffer',
             0x9008: 'ple_packet_buffer', 0xb000: 'haxi_config',
             0xb010: 'haxi_stop', 0x10000: 'cmac_functions'}


def summarize(records):
    values = {}
    for item in records:
        if (item.get('operation') == 'read' and item.get('status') == 0
                and item.get('width') == 4 and item.get('completed_bytes') == 4
                and item.get('address') in REGISTERS):
            value = item['value']
            if not isinstance(value, int) or not 0 <= value <= 0xffffffff:
                raise ValueError('Invalid register value')
            if value in (0xeaeaeaea, 0xdeadbeef):
                raise ValueError('Inaccessible register in transport snapshot')
            address = item['address']
            if address in values and values[address] != value:
                raise ValueError('Conflicting register observations')
            values[address] = value
    if set(values) != set(REGISTERS):
        raise ValueError('Incomplete transport snapshot')
    return {
        'registers': {REGISTERS[a]: f'0x{v:08x}' for a, v in values.items()},
        'hci_tx_dma_enabled': bool(values[0x7880] & 1),
        'hci_rx_dma_enabled': bool(values[0x7880] & 2),
        'rx_aggregation_enabled': bool(values[0x6000] & (1 << 31)),
        'cmac_enabled': bool(values[0x10000] & (1 << 30)),
        'haxi_dma_mode_code': (values[0xb000] >> 8) & 7,
        'haxi_tx_dma_enabled': bool(values[0xb000] & 16),
        'haxi_rx_dma_enabled': bool(values[0xb000] & 32),
        'wde_free_page_count': (values[0x8c08] >> 16) & 0x1fff,
        'ple_free_page_count': (values[0x9008] >> 16) & 0x1fff,
    }


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('Usage: transport_state.py REGISTER_TRACE.jsonl')
    with open(sys.argv[1]) as stream:
        print(json.dumps(summarize([json.loads(line) for line in stream]), indent=2))
