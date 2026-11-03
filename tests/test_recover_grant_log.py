import json
from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from recover_grant_log import recover

class RecoveryTests(unittest.TestCase):
    def fixture(self):
        parts=[]
        for phase,value in [('enabled',0x1757f5f),('restored',0x1775774)]:
            read=json.dumps(dict(operation='read',address=0xe348,width=4,value=value,status=0,completed_bytes=4),separators=(',',':'))
            parts.append('{"operation":"probe_grant_policy","phase":"'+phase+'","value":'+read+'\n'+str(value)+'}')
        return '\n'.join(parts)
    def test_exact_nested_read_reconstruction_and_jsonl(self):
        rows,repairs=recover(self.fixture());self.assertEqual(len(rows),4);self.assertEqual(len(repairs),2)
        self.assertEqual([r.get('operation') for r in rows],['read','probe_grant_policy']*2)
    def test_refuses_unrelated_corruption_or_unmatched_value(self):
        for text in [self.fixture().replace('24477535}', '24477536}'),self.fixture()+'\n{bad',self.fixture().splitlines()[0],
                     self.fixture().replace('"completed_bytes":4','"completed_bytes":3'),self.fixture()+ '\n'+self.fixture()]:
            with self.assertRaises(ValueError):recover(text)
