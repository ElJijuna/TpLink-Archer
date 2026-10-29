"""Consolidate recorded passive dwells; never opens USB or transmits."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def consolidate(dwells):
    if set(dwells) != {1, 6, 11}:
        raise ValueError('Expected exactly receive channels 1, 6, 11')
    groups = {}
    for channel, data in sorted(dwells.items()):
        if data.get('receive_channel') != channel or data.get('active_probes_sent') != 0:
            raise ValueError('Unexpected receive channel or active probe count')
        if data.get('bad_fcs') != 0 or data.get('table_overflow') != 0:
            raise ValueError('Capture has FCS errors or truncated network table')
        seen = set()
        accepted = 0
        for row in data['networks']:
            bssid, ssid = row['bssid_hex'], row['ssid_hex']
            if len(bytes.fromhex(bssid)) != 6 or len(bytes.fromhex(ssid)) > 32:
                raise ValueError('Invalid BSSID or SSID bytes')
            bssid, ssid = bssid.lower(), ssid.lower()
            ds = row['advertised_ds_channel']
            if type(ds) is not int or not 0 <= ds <= 255:
                raise ValueError('Invalid advertised channel')
            key = (bssid, ssid, ds)
            if key in seen:
                raise ValueError('Duplicate entry within dwell')
            seen.add(key)
            for field in ('beacons', 'probe_responses'):
                if type(row[field]) is not int or row[field] < 0:
                    raise ValueError('Invalid observation count')
            accepted += row['beacons'] + row['probe_responses']
            group = groups.setdefault((bssid, ssid), {'bssid_hex': bssid, 'ssid_hex': ssid,
                                                     'observations': []})
            # Keep counts and local observation ordinals scoped to their dwell.
            group['observations'].append({'receive_channel': channel, **row,
                                           'bssid_hex': bssid, 'ssid_hex': ssid})
        if accepted != data['accepted_management']:
            raise ValueError('Management count differs from network entries')
    return {'schema': 1, 'scope': 'recorded_passive_dwells',
            'receive_channels': [1, 6, 11], 'active_probes_sent': 0,
            'distinct_bssids': len({b for b, _ in groups}),
            'bssid_ssid_entries': len(groups),
            'networks': [groups[k] for k in sorted(groups)]}


def recorded_report(root, decoder):
    decoder = decoder.resolve()
    decoder_hash = hashlib.sha256(decoder.read_bytes()).hexdigest()
    dwells, sources = {}, []
    for channel in (1, 6, 11):
        capture = root / f'channel-{channel}'
        source = capture / 'networks.json'
        raw = source.read_bytes()
        live = json.loads(raw)
        replay = subprocess.run([str(decoder), f'--channel{channel}-fcs-appended', str(capture)],
                                check=True, capture_output=True, text=True)
        if live != json.loads(replay.stdout):
            raise ValueError(f'Channel {channel}: live/offline mismatch')
        dwells[channel] = live
        sources.append({'receive_channel': channel, 'networks_sha256': hashlib.sha256(raw).hexdigest(),
                        'live_equals_replay': True})
    return {**consolidate(dwells), 'decoder_sha256': decoder_hash, 'sources': sources,
            'limitations': ['Recorded observations, not a new hardware scan',
                            'BSSID/SSID entries are not physical access point counts',
                            'No association, network TX, or RF quality validation']}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture_root', type=Path)
    parser.add_argument('--decoder', type=Path,
                        default=Path(__file__).resolve().parents[1] / 'build/archer-passive-scan-decode-universal')
    args = parser.parse_args()
    print(json.dumps(recorded_report(args.capture_root, args.decoder), indent=2))
