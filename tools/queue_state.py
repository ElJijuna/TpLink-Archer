"""Offline DLE readiness and quota inspection. Readiness is not proof of bulk traffic."""
import json
import sys

ADDRESSES = (0x8d00, 0x9100, *range(0x8c40, 0x8c54, 4), *range(0x9040, 0x9074, 4))


def summarize(records):
    values = {}
    for r in records:
        if r.get('operation') != 'read' or r.get('address') not in ADDRESSES:
            continue
        if r.get('status') != 0 or r.get('width') != 4 or r.get('completed_bytes') != 4:
            raise ValueError('Failed or short queue-state read')
        address, value = r['address'], r['value']
        if not isinstance(value, int) or not 0 <= value <= 0xffffffff or value in (0xeaeaeaea, 0xdeadbeef):
            raise ValueError('Invalid or inaccessible queue register')
        if address in values and values[address] != value:
            raise ValueError('Conflicting queue reads')
        values[address] = value
    if set(values) != set(ADDRESSES):
        raise ValueError('Incomplete queue snapshot')
    quotas = {}
    for engine, base, count in (('wde', 0x8c40, 5), ('ple', 0x9040, 13)):
        quotas[engine] = [dict(index=i, address=base+4*i,
                               minimum=values[base+4*i] & 0xfff,
                               maximum=(values[base+4*i] >> 16) & 0xfff)
                          for i in range(count)]
    # Preserve anomalies rather than silently treating a boot profile as SCC.
    anomalies = [dict(engine=e, **q) for e, rows in quotas.items()
                 for q in rows if q['minimum'] > q['maximum']]
    return dict(wde_ready=(values[0x8d00] & 3) == 3,
                ple_ready=(values[0x9100] & 3) == 3,
                quotas=quotas, quota_anomalies=anomalies,
                h2c=quotas['ple'][3], c2h=quotas['ple'][2])


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('Usage: queue_state.py TRACE.jsonl')
    with open(sys.argv[1]) as stream:
        print(json.dumps(summarize([json.loads(x) for x in stream]), indent=2))
