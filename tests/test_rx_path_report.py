import copy
import unittest
from tools.rx_path_report import summarize


class RxPathTests(unittest.TestCase):
    def samples(self):
        a = dict(operation='rx_path_snapshot', elapsed_ms=0,
                 values=[0, 0, 0, 0, 0, 1, 2, 3, 7, 0x70000000],
                 ple_group0_empty=0xffffffff, quota6_raw=57)
        b = copy.deepcopy(a)
        b['elapsed_ms'] = 5000
        b['values'][5:8] = [11, 22, 33]
        return [a, b]

    def test_counts_and_limits(self):
        r = summarize(self.samples())
        self.assertEqual(r['phy_crc_ok_delta'], dict(cck=10, ofdm=20, ht=30))
        self.assertEqual(r['quota6_peak_used'], 0)
        self.assertEqual(r['ple_group0_nonempty_samples'], 0)
        self.assertIn('not continuous', r['limitation'])

    def test_flags_and_occupancy(self):
        rows = self.samples()
        rows[1]['values'][:4] = [4, 8, 16, 0x10000]
        rows[1]['values'][8:10] = [1, 0]
        rows[1]['quota6_raw'] = (4 << 16) | 57
        rows[1]['ple_group0_empty'] = 0xfffffffe
        r = summarize(rows)
        self.assertEqual(r['rmac_timeout_flags_or'], 4)
        self.assertEqual(r['host_rx_stop_samples'], 1)
        self.assertEqual(r['quota6_peak_used'], 4)
        self.assertEqual(r['hci_rx_disabled_samples'], 1)
        self.assertEqual(r['cmac_rx_disabled_samples'], 1)

    def test_saturation_or_counter_reset_is_unknown(self):
        rows = self.samples()
        rows[1]['values'][5] = 0xffff
        rows[1]['values'][6] = 1
        r = summarize(rows)
        self.assertIsNone(r['phy_crc_ok_delta']['cck'])
        self.assertIsNone(r['phy_crc_ok_delta']['ofdm'])

    def test_invalid_samples(self):
        for bad in [[], self.samples()[:1]]:
            with self.assertRaises(ValueError):
                summarize(bad)
        for field, value in [('values', [0]), ('elapsed_ms', -1), ('quota6_raw', 0xdeadbeef)]:
            rows = self.samples()
            rows[1][field] = value
            with self.assertRaises(ValueError):
                summarize(rows)
