import importlib.util
import unittest
spec = importlib.util.spec_from_file_location('queue_state', 'tools/queue_state.py')
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


class QueueTests(unittest.TestCase):
    def records(self):
        return [dict(operation='read', address=a, value={0x8d00:3, 0x9100:3,
                    0x904c:0x0b5f0100, 0x9048:0x00200010, 0x8c44:6}.get(a,0),
                    status=0, width=4, completed_bytes=4) for a in m.ADDRESSES]

    def test_boot_snapshot(self):
        r = m.summarize(self.records())
        self.assertTrue(r['wde_ready'] and r['ple_ready'])
        self.assertEqual((r['h2c']['minimum'],r['h2c']['maximum']), (256,2911))
        self.assertEqual(len(r['quota_anomalies']),1)
        self.assertEqual(r['quota_anomalies'][0]['address'],0x8c44)

    def test_not_ready(self):
        records=self.records()
        records[0]['value']=1
        self.assertFalse(m.summarize(records)['wde_ready'])

    def test_bad_snapshots(self):
        for update in ({'value':0xeaeaeaea},{'status':-1},{'completed_bytes':0}):
            records=self.records()
            records[0].update(update)
            with self.assertRaises(ValueError): m.summarize(records)
        with self.assertRaises(ValueError): m.summarize(self.records()[:-1])
        records=self.records()
        records.append(dict(records[0],value=0))
        with self.assertRaises(ValueError): m.summarize(records)
