import importlib.util
import unittest
spec = importlib.util.spec_from_file_location('efuse_map', 'tools/efuse_map.py')
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


def entry(page, block, mask, payload):
    return ((page << 17) | (block << 4) | mask).to_bytes(3, 'big') + payload


def image(entries):
    return (b'\0'*4 + entries).ljust(0x1300, b'\xff')


class EfuseTests(unittest.TestCase):
    def test_page_filter_overwrite_and_unprogrammed(self):
        raw = image(entry(1, 0, 14, b'AB') + entry(3, 0, 14, b'XY') +
                    entry(1, 0, 14, b'CD'))
        self.assertEqual(m.decode_block(raw, 1, 0, 4), b'CD\xff\xff')
        self.assertEqual(m.decode_block(raw, 3, 0, 2), b'XY')

    def test_odd_range_and_mask(self):
        raw = image(entry(1, 1, 10, b'ABCD')) # words 0 and 2 enabled.
        self.assertEqual(m.decode_block(raw, 1, 9, 5), b'B\xff\xffCD')

    def test_bounds(self):
        for args in ((b'', 1, 0, 4), (image(b''), 7, 0, 2),
                     (image(b''), 1, 65535, 2)):
            with self.assertRaises(ValueError): m.decode_block(*args)
        # Repeated valid empty entries end with a header needing a missing word.
        raw = b'\0'*4 + b'\0\0\x0f'*1619 + b'\0\0\x0e'
        self.assertEqual(len(raw), 0x1300)
        with self.assertRaises(ValueError): m.decode_block(raw, 1, 0, 2)
