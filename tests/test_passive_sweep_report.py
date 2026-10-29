import importlib.util
from pathlib import Path
import unittest
import copy
spec = importlib.util.spec_from_file_location('report', Path(__file__).parents[1] / 'tools/passive_sweep_report.py')
r = importlib.util.module_from_spec(spec)
spec.loader.exec_module(r)


def dwell(channel, ds):
    return dict(receive_channel=channel, active_probes_sent=0, bad_fcs=0, table_overflow=0,
                accepted_management=2, networks=[dict(bssid_hex='001122334455', ssid_hex='ff00',
                    advertised_ds_channel=ds, beacons=1, probe_responses=1,
                    first_observation=1, last_observation=2)])


class SweepReportTest(unittest.TestCase):
    def test_provenance_and_binary_ssid(self):
        data={c:dwell(c,5) for c in (1,6,11)}
        original=copy.deepcopy(data)
        result=r.consolidate(data)
        self.assertEqual(result['distinct_bssids'],1)
        self.assertEqual(result['bssid_ssid_entries'],1)
        rows=result['networks'][0]['observations']
        self.assertEqual([x['receive_channel'] for x in rows],[1,6,11])
        self.assertTrue(all(x['advertised_ds_channel']==5 for x in rows))
        self.assertEqual(result['networks'][0]['ssid_hex'],'ff00')
        self.assertEqual(data,original)

    def test_hidden_and_named_remain_separate(self):
        data={c:dwell(c,c) for c in (1,6,11)}
        data[1]['networks'][0]['ssid_hex']=''
        result=r.consolidate(data)
        self.assertEqual(result['distinct_bssids'],1)
        self.assertEqual(result['bssid_ssid_entries'],2)

    def test_reject_invalid_or_incomplete_capture(self):
        for field,value in [('receive_channel',11),('active_probes_sent',1),
                            ('bad_fcs',1),('table_overflow',1),('accepted_management',3)]:
            data={c:dwell(c,c) for c in (1,6,11)}
            data[1][field]=value
            with self.assertRaises(ValueError):r.consolidate(data)
        with self.assertRaises(ValueError):r.consolidate({1:dwell(1,1)})
        data={c:dwell(c,c) for c in (1,6,11)}
        data[1]['networks']*=2
        with self.assertRaises(ValueError):r.consolidate(data)
