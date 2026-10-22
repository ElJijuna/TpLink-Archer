"""Bounded offline observations; CRC counters do not prove RMAC delivery."""
import json
import sys


def summarize(records):
    samples = [r for r in records if r.get('operation') == 'rx_path_snapshot']
    if not 2 <= len(samples) <= 100:
        raise ValueError('Expected 2..100 RX path samples')
    previous = -1
    for r in samples:
        elapsed, values = r.get('elapsed_ms'), r.get('values')
        if type(elapsed) is not int or not previous <= elapsed <= 15000:
            raise ValueError('Invalid or decreasing sample time')
        previous = elapsed
        if not isinstance(values, list) or len(values) != 10:
            raise ValueError('Incomplete RX path snapshot')
        for value in values + [r.get('ple_group0_empty'), r.get('quota6_raw')]:
            if type(value) is not int or not 0 <= value <= 0xffffffff or value in (0xdeadbeef, 0xeaeaeaea):
                raise ValueError('Invalid or inaccessible snapshot value')
    deltas = {}
    for name, index in [('cck', 5), ('ofdm', 6), ('ht', 7)]:
        counts = [r['values'][index] & 0xffff for r in samples]
        valid = max(counts) < 0xffff and all(a <= b for a, b in zip(counts, counts[1:]))
        deltas[name] = counts[-1] - counts[0] if valid else None
    flags = [0, 0, 0]
    for r in samples:
        for i in range(3):
            flags[i] |= r['values'][i]
    return dict(samples=len(samples), elapsed_ms=samples[-1]['elapsed_ms'] - samples[0]['elapsed_ms'],
                phy_crc_ok_delta=deltas, rmac_timeout_flags_or=flags[0],
                rx_dma_flags_or=flags[1], rx_dma_flags1_or=flags[2],
                host_rx_stop_samples=sum(bool(r['values'][3] & 0x1003f) for r in samples),
                hci_rx_disabled_samples=sum(not bool(r['values'][8] & 2) for r in samples),
                cmac_rx_disabled_samples=sum(not bool(r['values'][9] & 0x10000000) for r in samples),
                ple_group0_nonempty_samples=sum(r['ple_group0_empty'] != 0xffffffff for r in samples),
                quota6_peak_used=max((r['quota6_raw'] >> 16) & 0xfff for r in samples),
                limitation='Sequential sampled reads, not continuous or atomic; no inferred RMAC packet count')


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('Usage: rx_path_report.py TRACE.jsonl')
    with open(sys.argv[1]) as stream:
        print(json.dumps(summarize([json.loads(line) for line in stream]), indent=2))
