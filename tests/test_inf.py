import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('inspect_inf', Path(__file__).parents[1] / 'tools/inspect_inf.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class InfTests(unittest.TestCase):
    def parse(self, text, encoding):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'driver.inf'
            path.write_bytes(text.encode(encoding))
            return module.inspect(path)

    def test_utf16_hardware_match_and_version(self):
        result = self.parse('[Version]\nDriverVer=11/18/2025,5102.24.126.7\n[Models.NTamd64]\n%device%=RTL8922au.ndi, USB\\VID_37AD&PID_0104\n', 'utf-16')
        self.assertEqual(result['matches'][0]['install_section'], 'RTL8922au.ndi')
        self.assertEqual(result['driver_version'], '11/18/2025,5102.24.126.7')

    def test_no_prefix_or_comment_false_matches(self):
        result = self.parse('[Models]\n; %x%=bad, USB\\VID_37AD&PID_0104\n%x%=bad, USB\\VID_37AD&PID_01040\n', 'utf-8')
        self.assertEqual(result['matches'], [])

    def test_case_and_interface_suffix(self):
        result = self.parse('[Models]\n%x%=ok, usb\\vid_37ad&pid_0104&mi_00\n', 'utf-8')
        self.assertEqual(len(result['matches']), 1)

if __name__ == '__main__':
    unittest.main()
