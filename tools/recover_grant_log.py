"""Recover only the exact tag9 nested-read JSONL defect; preserve original bytes."""
import argparse
import hashlib
import json
from pathlib import Path
import re


def recover(text):
    lines=text.splitlines();rows=[];repairs=[];i=0
    pattern=re.compile(r'^\{"operation":"probe_grant_policy","phase":"(enabled|restored)","value":(\{.*\})$')
    while i<len(lines):
        line=lines[i]
        try:row=json.loads(line)
        except json.JSONDecodeError:
            match=pattern.fullmatch(line)
            if match is None or i+1>=len(lines):raise ValueError(f'Unexpected malformed line{i+1}')
            read=json.loads(match[2]);tail=re.fullmatch(r'(\d+)\}',lines[i+1])
            if (tail is None or set(read)!=set(('operation','address','width','value','status','completed_bytes')) or
                read.get('operation')!='read' or read.get('address')!=0xe348 or read.get('width')!=4 or
                read.get('status')!=0 or read.get('completed_bytes')!=4 or read['value']!=int(tail[1])):
                raise ValueError('Nested grant read does not agree with trailing value')
            phase=match[1]
            if phase in [r['phase'] for r in repairs]:raise ValueError('Repeated grant phase')
            rows.extend((read,dict(operation='probe_grant_policy',phase=phase,value=read['value'])))
            repairs.append(dict(first_line=i+1,second_line=i+2,phase=phase,value=read['value']))
            i+=2;continue
        if not isinstance(row,dict):raise ValueError('Expected JSON object')
        rows.append(row);i+=1
    if [r['phase'] for r in repairs]!=['enabled','restored']:raise ValueError('Exact two tag9 repairs required')
    if [r['value'] for r in repairs]!=[0x1757f5f,0x1775774]:raise ValueError('Unexpected grant policy values')
    return rows,repairs


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);p.add_argument('new_output',type=Path)
    a=p.parse_args();data=a.source.read_bytes();rows,repairs=recover(data.decode())
    with a.new_output.open('x') as out:
        for row in rows:out.write(json.dumps(row,separators=(',',':'))+'\n')
    print(json.dumps(dict(source_sha256=hashlib.sha256(data).hexdigest(),
        derived_sha256=hashlib.sha256(a.new_output.read_bytes()).hexdigest(),repairs=repairs),indent=2))
