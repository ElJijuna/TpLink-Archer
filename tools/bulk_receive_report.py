"""IOKit ReadPipeTO telemetry: size on error is not an actual byte count."""
import json
import sys


def summarize(records):
    rows = [r for r in records if r.get('operation') == 'bulk_receive_result']
    if not rows:
        raise ValueError('No bulk receive observations')
    endpoints = {}
    for r in rows:
        endpoint = r.get('endpoint')
        status, size, requested = r.get('status'), r.get('reported_size'), r.get('requested_bytes')
        if endpoint not in (0x84, 0x88) or type(status) is not int:
            raise ValueError('Invalid endpoint/status')
        if type(size) is not int or type(requested) is not int or not 0 <= size <= 0xffffffff or not 0 < requested <= 20480:
            raise ValueError('Invalid receive lengths')
        if type(r.get('size_valid')) is not bool or r['size_valid'] != (status == 0) or type(r.get('timeout')) is not bool:
            raise ValueError('Invalid IOKit size validity')
        if (status == 0 and (size > requested or r['timeout'])):
            raise ValueError('Inconsistent successful receive')
        item = endpoints.setdefault(hex(endpoint), dict(reads=0, successes=0, valid_bytes=0, timeouts=0, other_errors=0, error_size_unchanged=0))
        item['reads'] += 1
        if status == 0:
            item['successes'] += 1
            item['valid_bytes'] += size
        else:
            item['timeouts' if r['timeout'] else 'other_errors'] += 1
            item['error_size_unchanged'] += size == requested
    return dict(endpoints=endpoints, limitation='No bytes counted from failed reads; telemetry does not prove absence of device-side data or data-toggle synchronization')


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('Usage: bulk_receive_report.py TRACE.jsonl')
    with open(sys.argv[1]) as f:
        print(json.dumps(summarize([json.loads(line) for line in f]), indent=2))
