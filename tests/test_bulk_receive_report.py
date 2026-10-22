import unittest
from tools.bulk_receive_report import summarize


class BulkReportTests(unittest.TestCase):
    def row(self, **fields):
        return dict(dict(operation='bulk_receive_result', endpoint=0x84, status=-1,
                         requested_bytes=20480, reported_size=20480, size_valid=False, timeout=True), **fields)

    def test_failed_size_never_counted(self):
        r = summarize([self.row(), self.row(reported_size=123)])['endpoints']['0x84']
        self.assertEqual(r['valid_bytes'], 0)
        self.assertEqual(r['timeouts'], 2)
        self.assertEqual(r['error_size_unchanged'], 1)

    def test_success_and_error_separate_endpoints(self):
        r = summarize([self.row(), self.row(endpoint=0x88, status=0, reported_size=64, size_valid=True, timeout=False), self.row(timeout=False)])
        self.assertEqual(r['endpoints']['0x88']['valid_bytes'], 64)
        self.assertEqual(r['endpoints']['0x84']['other_errors'], 1)

    def test_invalid_observations(self):
        with self.assertRaises(ValueError):
            summarize([])
        for fields in [dict(size_valid=True), dict(endpoint=1), dict(status=0, size_valid=True, timeout=False, reported_size=20481)]:
            with self.assertRaises(ValueError):
                summarize([self.row(**fields)])
