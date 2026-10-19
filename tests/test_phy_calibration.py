import importlib.util
import unittest
spec=importlib.util.spec_from_file_location('phy_calibration','tools/phy_calibration.py')
m=importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


class PhyTests(unittest.TestCase):
    def test_signed_magnitude_and_bias_nibbles(self):
        b=bytearray(b'\xff'*56)
        b[0]=254;b[6]=0x81;b[0x33]=0xc0
        b[7]=0x10;b[8]=0x31;b[0x34]=0x2e;b[0x35]=0x12
        r=m.decode(b)
        self.assertTrue(r['thermal_programmed'])
        self.assertTrue(r['pa_and_pad_programmed'])
        self.assertEqual([p['thermal_trim'] for p in r['paths']],[-1,0])
        self.assertEqual(r['paths'][1]['pa_2g'],14)
        self.assertEqual(r['paths'][0]['pad_5g'],3)

    def test_erased_gates_and_partial_thermal(self):
        r=m.decode(b'\xff'*56)
        self.assertFalse(r['thermal_programmed'])
        self.assertFalse(r['pa_and_pad_programmed'])
        b=bytearray(b'\xff'*56);b[6]=0x43
        r=m.decode(b)
        self.assertFalse(r['thermal_programmed'])
        self.assertEqual(r['paths'][0]['thermal_trim'],3)
        self.assertFalse(r['rf_writes_applied'])

    def test_length(self):
        with self.assertRaises(ValueError):m.decode(b'\0'*57)
