import importlib.util
from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path('tools').resolve()))
spec=importlib.util.spec_from_file_location('phy_table','tools/phy_table.py')
m=importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


class PhyTableTests(unittest.TestCase):
    def test_headline_priority_and_fallback(self):
        pairs=[(0xf0010000,0),(0xf0010001,0),(0xf0010002,0),(0xf0ff0003,0)]
        self.assertEqual(m.select_headline(pairs,1,1),(4,0x10001))
        self.assertEqual(m.select_headline(pairs,1,9),(4,0x10002))
        self.assertEqual(m.select_headline(pairs,4,1),(4,0xff0003))
        pairs.insert(1,(0xf00100ff,0))
        self.assertEqual(m.select_headline(pairs,1,9)[1],0x100ff)
        with self.assertRaises(ValueError):m.select_headline([(0xf0020001,0)],1,1)

    def test_elif_then_common_and_else_skip(self):
        pairs=[(0xf0010001,0),(0x80010000,0),(0x40000000,0),(0x1000,10),
               (0x90010001,0),(0x40000000,0),(0x1004,20),
               (0xa0000000,0),(0x1008,30),(0xb0000000,0),(0x100c,40)]
        p=m.interpret(pairs,1,1,'bb')
        self.assertEqual([x['value'] for x in p['operations']],[20,40])

    def test_callbacks_distinguish_rf_v1_and_bb(self):
        pairs=[(0xfe,0),(0xfd,0),(0xfa,0),(0x1000,0xbabecafe)]
        bb=m.interpret(pairs,1,1,'bb')['operations']
        rf=m.interpret(pairs,1,1,'rf_v1')['operations']
        self.assertEqual([x['kind'] for x in bb],['delay','delay','delay','bypass'])
        self.assertEqual([x['kind'] for x in rf],['delay','rf_write','rf_write','rf_write'])
        self.assertFalse(rf[1]['rf_firmware_store'])
        self.assertTrue(rf[-1]['rf_firmware_store'])

    def test_malformed_programs_fail(self):
        programs=[[(0x40000000,0)],[(0xf0010001,0),(0x80010001,0)],
                  [(0xf0010001,0),(0x80010001,0),(0x1000,1)],
                  [(0xb0000000,0)],[(0xc0000000,0)],
                  [(0xf0010001,0),(0x80010002,0),(0x40000000,0),(0xa0000000,0)]]
        for pairs in programs:
            with self.assertRaises(ValueError):m.interpret(pairs,1,1,'bb')
