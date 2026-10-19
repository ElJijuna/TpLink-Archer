import importlib.util
import unittest

spec = importlib.util.spec_from_file_location('transport_state', 'tools/transport_state.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class TransportStateTests(unittest.TestCase):
    def records(self):
        return [dict(operation='read', status=0, width=4, completed_bytes=4,
                     address=a, value=0) for a in module.REGISTERS]

    def test_bits_and_fields(self):
        records = self.records()
        for r in records:
            r['value'] = {0x7880: 7, 0xb000: 0x80436,
                          0x9008: 0x0b701a01}.get(r['address'], 0)
        result = module.summarize(records)
        self.assertTrue(result['hci_tx_dma_enabled'])
        self.assertTrue(result['haxi_rx_dma_enabled'])
        self.assertEqual(result['haxi_dma_mode_code'], 4)
        self.assertEqual(result['ple_free_page_count'], 2928)
        self.assertFalse(result['cmac_enabled'])

    def test_reject_missing_failed_short_and_sentinel(self):
        for update in ({'status': -1}, {'completed_bytes': 3}, {'value': 0xeaeaeaea}):
            records = self.records()
            records[0].update(update)
            with self.assertRaises(ValueError):
                module.summarize(records)
        with self.assertRaises(ValueError):
            module.summarize(self.records()[:-1])

    def test_reject_conflicting_reads(self):
        records = self.records()
        records.append(dict(records[0], value=1))
        with self.assertRaises(ValueError):
            module.summarize(records)
