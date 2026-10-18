import importlib.util
from pathlib import Path
import struct
import unittest

spec = importlib.util.spec_from_file_location('firmware', Path(__file__).parents[1] / 'tools/firmware.py')
firmware = importlib.util.module_from_spec(spec)
spec.loader.exec_module(firmware)

def sample(dynamic=True, checksum=False):
    header_size = 72 if dynamic else 64
    words = [0x89227102, 0x0f2f2300, 0xe28be105, 1 << 24, 0,
             header_size << 16 | 2025, 1 << 8, 2020 | ((1 << 16) if dynamic else 0),
             0, 0, 0, 0]
    flags = 2 << 24 | 4 | ((1 << 28) if checksum else 0)
    data = struct.pack('<12I4I', *words, 0x20100000, flags, 0, 0)
    if dynamic:
        data += struct.pack('<2I', 8, 0)
    return data + b'test' + (b'12345678' if checksum else b'')

class FirmwareTests(unittest.TestCase):
    def test_dynamic_and_static_headers(self):
        for dynamic in (True, False):
            result = firmware.inspect_v1(sample(dynamic))
            self.assertEqual(result['version'], '0.35.47.15')
            self.assertEqual(result['sections'][0]['payload_bytes'], 4)
            self.assertFalse(result['signature_verified'])

    def test_checksum_storage_is_counted_without_claiming_verification(self):
        result = firmware.inspect_v1(sample(checksum=True))
        self.assertEqual(result['sections'][0]['stored_bytes'], 12)
        self.assertEqual(result['validation'], 'structure_only')

    def test_every_truncated_prefix_and_trailing_bytes(self):
        data = sample()
        for size in range(len(data)):
            with self.subTest(size=size), self.assertRaises(ValueError):
                firmware.inspect_v1(data[:size])
        with self.assertRaises(ValueError):
            firmware.inspect_v1(data + b'extra')

    def test_invalid_table_dynamic_header_and_security(self):
        for offset, value in ((24, 0xff00), (20, 0xffff07e9), (64, 7),
                              (52, 0x09000004), (12, 2 << 24)):
            data = bytearray(sample())
            struct.pack_into('<I', data, offset, value)
            if offset == 52:
                struct.pack_into('<I', data, 56, 1)
            with self.subTest(offset=offset), self.assertRaises(ValueError):
                firmware.inspect_v1(data)

    def test_rejects_wrong_carrier_before_extraction(self):
        with self.assertRaisesRegex(ValueError, 'SHA-256'):
            firmware.extract_bound_images(b'MZwrong')

    def test_invalid_pe_mappings(self):
        for data in (b'', b'MZ', bytes(128)):
            with self.assertRaises(ValueError):
                firmware.pe_file_offset(data, 0x140000000, 4)

if __name__ == '__main__':
    unittest.main()
