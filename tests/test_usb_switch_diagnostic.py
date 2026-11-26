import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from diagnose_usb_switch import parse_devices

class USBTimelineTest(unittest.TestCase):
    def test_only_supported_usb_devices_and_own_properties(self):
        text = '''+-o DISK <class IOUSBHostDevice, id 123>
  |   "idVendor" = 3034
  |   "idProduct" = 6699
  |   "sessionID" = 99
  +-o Child <class Other>
  |     "idVendor" = 14253
+-o Mouse <class IOUSBHostDevice, id 456>
  |   "idVendor" = 111
  |   "idProduct" = 260
'''
        self.assertEqual(parse_devices(text), [dict(idVendor=3034,idProduct=6699,sessionID=99,mode='installer')])

    def test_wifi_and_absence(self):
        self.assertEqual(parse_devices(''), [])
        text = '+-o WLAN <class IOUSBHostDevice, id 1>\n  |   "idVendor" = 14253\n  |   "idProduct" = 260\n'
        self.assertEqual(parse_devices(text)[0]['mode'], 'wifi')

class USBContrastTest(unittest.TestCase):
    def test_bulk_failure_is_observed_without_fallback_or_retry(self):
        import contextlib
        import io
        import json
        import tempfile
        from unittest.mock import patch
        import subprocess
        import diagnose_usb_switch as diagnostic
        state = [dict(idVendor=3034,idProduct=6699,sessionID=7,mode='installer')]
        calls = []
        def run(command, **kwargs):
            calls.append(command)
            if '--eject-only-installer' in command:
                return subprocess.CompletedProcess(command, 2, 'CSW mismatch', '')
            return subprocess.CompletedProcess(command, 0, 'verified', '')
        ticks = iter(range(1000))
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp)/'capture'
            with patch.object(sys,'argv',['diagnose', '--current','--bulk-eject',str(output)]), patch.object(diagnostic,'snapshot',return_value=state), patch.object(diagnostic.subprocess,'run',side_effect=run), patch.object(diagnostic.time,'sleep'), patch.object(diagnostic.time,'monotonic',side_effect=lambda: next(ticks)), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(diagnostic.main(),0)
            rows = [json.loads(line) for line in (output/'timeline.jsonl').read_text().splitlines()]
            self.assertEqual(sum('--eject-only-installer' in call for call in calls),1)
            self.assertFalse(any('--eject-installer' in call for call in calls))
            self.assertTrue(any(row['operation']=='command_unconfirmed' for row in rows))
            self.assertFalse(rows[-1]['saw_wifi'])

if __name__ == '__main__':
    unittest.main()
