import importlib.util
import unittest
spec=importlib.util.spec_from_file_location('rf_calibration','tools/rf_calibration.py')
m=importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


class RfTests(unittest.TestCase):
    def test_packed_offsets(self):
        data=bytearray(b'\xff'*0x240)
        for address,value in ((0xb8,1),(0xb9,2),(0xc1,3),(0xca,4),(0xd0,5),(0xd1,6),
                              (0x10,7),(0x3a,8),(0x100,9),(0x12a,10),(0x1a8,11),(0x1b0,12)):
            data[address]=value
        r=m.decode(data)
        self.assertEqual(r['rfe_type_raw'],4)
        self.assertEqual(r['crystal_trim_raw'],2)
        self.assertEqual(r['paths'][0]['tssi_cck_raw'][0],7)
        self.assertEqual(r['paths'][1]['tssi_cck_raw'][0],8)
        self.assertEqual(r['paths'][1]['tssi_6g_raw'][0],10)
        self.assertEqual(r['paths'][1]['rx_gain_6g_raw'][0],12)
        self.assertEqual(r['paths'][1]['thermal_raw'],6)

    def test_erased_and_length(self):
        self.assertFalse(m.decode(b'\xff'*0x240)['factory_map_populated'])
        with self.assertRaises(ValueError):m.decode(b'\0'*10)
