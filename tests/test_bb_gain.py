import importlib.util
import sys
from pathlib import Path
import unittest
sys.path.insert(0,str(Path('tools').resolve()))
spec=importlib.util.spec_from_file_location('bb_gain','tools/bb_gain.py')
m=importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


class GainTests(unittest.TestCase):
    def test_signed_gain_and_path_bw(self):
        r=m.decode([(0x00031100,0x807ffe01),(0x00031101,0x030201)])
        fields=r['fields']
        self.assertEqual([x['value'] for x in fields],[1,-2,127,-128,1,2,3])
        self.assertTrue(all(x['band']==3 and x['bw_type']==1 and x['path']==1 for x in fields))

    def test_rpl_halves_and_overwrite(self):
        r=m.decode([(0x01000130,0x04030201),(0x01000131,0x08070605),
                    (0x01000130,0x0c0b0a09)])
        self.assertEqual([x['value']for x in r['fields']],[9,10,11,12,5,6,7,8])
        self.assertTrue(all(x['bw_type'] is None for x in r['fields']))

    def test_bypass_efem_and_masked_candidate(self):
        r=m.decode([(0x02000000,0),(0x04000000,0),(0x0f001000,0x1234)])
        self.assertEqual(len(r['ignored']),2)
        self.assertEqual(r['register_candidates'][0]['mask'],0xffff0000)
        self.assertEqual(r['register_candidates'][0]['bb_address'],0x1000)
        with self.assertRaises(ValueError):m.decode([(0x04000000,0)],50)

    def test_bounds_unknown_and_flow_control(self):
        self.assertEqual(len(m.decode([(0x00002000,0)])['ignored']),1)
        for pair in ((0xf9,0),(0x05000000,0),(0x00000004,0)):
            with self.assertRaises(ValueError):m.decode([pair])
