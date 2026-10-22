import copy
import unittest
from tools.rx_path_report import summarize, summarize_phy_mac


class RxPathTests(unittest.TestCase):
    def test_phy_mac_flags_and_config(self):
        a = dict(operation='phy_mac_status', phase='before', values=[0, 0, 0, 0x59, 0, 0, 0, 0, 0, 1, 0x430, 0, 0, 0, 0, 0x70000020])
        b = copy.deepcopy(a)
        b['phase'] = 'after'
        b['values'][3] |= 0x100
        b['values'][8] = 1
        b['values'][9] |= 4
        b['values'][12] = 0x20
        r = summarize_phy_mac([a, b])
        self.assertEqual(r['rmac_new_bits_after_first'], 0x100)
        self.assertEqual(r['loopback_enabled_samples'], 1)
        self.assertEqual(r['drop_nondma_ppdu_samples'], 1)
        self.assertEqual(r['haxi_dma_mode_codes'], [4])
        self.assertEqual(r['haxi_rx_disabled_samples'], 0)
        self.assertEqual(r['dispatcher_flags_or'], [0x20, 0, 0])

    def test_phy_mac_unchanged_flags_and_invalid_input(self):
        row = dict(operation='phy_mac_status', phase='before', values=[0] * 16)
        row['values'][3] = 0x59
        r = summarize_phy_mac([row, copy.deepcopy(row)])
        self.assertEqual(r['rmac_new_bits_after_first'], 0)
        self.assertIn('cannot be dated', r['limitation'])
        with self.assertRaises(ValueError):
            summarize_phy_mac([])
        for change in [dict(values=[0]), dict(phase=''), dict(values=[0xdeadbeef] * 16)]:
            bad = dict(row, **change)
            with self.assertRaises(ValueError):
                summarize_phy_mac([bad])
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
