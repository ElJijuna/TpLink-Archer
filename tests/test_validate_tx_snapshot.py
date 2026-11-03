import copy
from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path(__file__).parents[1]/'tools'))
from validate_tx_snapshot import validate_trace


def fixture():
    def transfer(op,address,value):return dict(operation=op,address=address,value=value,width=4,status=0,completed_bytes=4)
    rows=[transfer('read',0x74,123),transfer('write',0x74,456)]
    rows += [transfer('write',0x270,v) for v in (0x81000041,0x810000fd,0x810000fe)]
    rows += [dict(operation='tx_power_snapshot',address=0x11000+i*4,mask=255,
                  observed=0x1205,expected_host_plan=5,matches_host_plan=True) for i in range(210)]
    rows += [dict(operation='passive_phy_tx_counters',sample=i,controls_enabled=True,counts=[0,0,0,0]) for i in range(3)]
    rows += [dict(result='tx_power_snapshot_complete',registers=210,differences_from_host_plan=0,
                  network_tx_paused=True,h2c_submissions=0,firmware_resets=0),
             transfer('write',0x74,123),transfer('read',0x74,123)]
    return rows


class TxSnapshotValidationTest(unittest.TestCase):
    def test_accepts_complete_trace(self):self.assertEqual(len(validate_trace(fixture())),210)

    def test_rejects_short_failed_or_unrestored_access(self):
        for field,value in (('status',1),('completed_bytes',3)):
            rows=fixture();rows[0][field]=value
            with self.assertRaises(ValueError):validate_trace(rows)
        rows=fixture();rows[-1]['value']=999
        with self.assertRaises(ValueError):validate_trace(rows)

    def test_rejects_any_power_write_or_xtal_write_opcode(self):
        rows=fixture();rows.insert(3,dict(operation='write',address=0x11e00,value=0,width=4,status=0,completed_bytes=4))
        with self.assertRaises(ValueError):validate_trace(rows)
        rows=fixture();rows[2]['value']=0x82000041
        with self.assertRaises(ValueError):validate_trace(rows)

    def test_rejects_missing_duplicate_and_mismatched_register(self):
        original=fixture();index=next(i for i,r in enumerate(original) if r.get('operation')=='tx_power_snapshot')
        for change in ('missing','duplicate','mismatch'):
            rows=copy.deepcopy(original)
            if change=='missing':del rows[index]
            elif change=='duplicate':rows[index+1]['address']=rows[index]['address']
            else:rows[index]['observed']=6
            with self.assertRaises(ValueError):validate_trace(rows)

    def test_rejects_counter_change_disabled_and_incomplete_cleanup(self):
        for change in ('change','disabled','missing_cleanup'):
            rows=fixture()
            if change=='missing_cleanup':rows=[r for r in rows if 'result' not in r]
            else:
                row=next(r for r in rows if r.get('operation')=='passive_phy_tx_counters' and r['sample']==2)
                if change=='change':row['counts']=[1,0,0,0]
                else:row['controls_enabled']=False
            with self.assertRaises(ValueError):validate_trace(rows)
