"""Inspect Windows INF model entries without loading or executing a driver."""
import argparse
import hashlib
import json
from pathlib import Path
import re

TARGET = re.compile(r"USB\\VID_37AD&PID_0104(?:&[A-Z0-9_]+)*$", re.I)

def inspect(path):
    raw = Path(path).read_bytes()
    if raw.startswith((b'\xff\xfe', b'\xfe\xff')):
        text = raw.decode('utf-16')
    else:
        try:
            text = raw.decode('utf-8-sig')
        except UnicodeDecodeError:
            text = raw.decode('cp1252')
    section = None
    matches = []
    version = None
    for line_number, line in enumerate(text.splitlines(), 1):
        line = line.split(';', 1)[0].strip()
        if line.startswith('[') and line.endswith(']'):
            section = line[1:-1]
        if '=' not in line:
            continue
        key, value = (part.strip() for part in line.split('=', 1))
        if section and section.lower() == 'version' and key.lower() == 'driverver':
            version = value
        fields = [field.strip() for field in value.split(',')]
        # IDs in comments or arbitrary strings do not establish model matching.
        if section and section.lower().startswith('manufacturer'):
            continue
        if len(fields) >= 2 and any(TARGET.fullmatch(f) for f in fields[1:]):
            matches.append({'line': line_number, 'section': section,
                            'description_token': key, 'install_section': fields[0],
                            'hardware_ids': fields[1:]})
    return {'path': str(path), 'sha256': hashlib.sha256(raw).hexdigest(),
            'driver_version': version, 'matches': matches}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('path', type=Path)
    args = parser.parse_args()
    paths = sorted(args.path.rglob('*.inf')) if args.path.is_dir() else [args.path]
    results = [inspect(path) for path in paths]
    print(json.dumps(results, indent=2))
    return 0 if any(row['matches'] for row in results) else 1

if __name__ == '__main__':
    raise SystemExit(main())
