import importlib.util
import unittest

spec = importlib.util.spec_from_file_location('dle_plan', 'tools/dle_plan.py')
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


class DlePlanTests(unittest.TestCase):
    def test_usb2_memory_and_h2c_quota(self):
        plan = m.candidate()
        entries = {x['address']: x for x in plan['register_candidates']}
        self.assertEqual(plan['fifo_bytes'], 589824)
        self.assertEqual(entries[0x8c08]['value'], 0x01fc0000)
        self.assertEqual(entries[0x9008]['value'], 0x080b0402)
        self.assertEqual(entries[0x904c]['value'], 0x00180018)
        self.assertEqual(entries[0x9048]['value'], 0x00100010)
        self.assertEqual(len(entries), 20)
        self.assertFalse(plan['executable'])

    def test_download_layout_matches_physical_ple(self):
        self.assertEqual(m.packet_buffer(128, 2928, 212992, 'ple'), 0x0b701a01)
        self.assertEqual(m.packet_buffer(64, 0, 0, 'wde'), 0)

    def test_invalid_layout_and_quota(self):
        for args in ((256, 1, 0, 'wde'), (64, 1, 0, 'ple'),
                     (256, 8192, 0, 'ple'), (256, 1, 1, 'ple')):
            with self.assertRaises(ValueError):
                m.packet_buffer(*args)
        for args in ((2, 1), (-1, 2), (1, 4096)):
            with self.assertRaises(ValueError):
                m.quota(*args)
