import sys
import struct
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parents[1]/'tools'))
from firmware_download import packets, usb_frame, prepare
from test_firmware import sample
from test_bbmcu import container, bb


class DownloadTests(unittest.TestCase):
    def test_header_omits_dynamic_extension_and_sections_are_raw(self):
        image=sample()
        frames=list(packets(image,9))
        header=frames[0][2]
        self.assertEqual(struct.unpack_from('<I',header)[0],13<<24|72)
        self.assertEqual(header[24:32],struct.pack('<II',0x0900000d,72))
        self.assertEqual(header[32:],image[:64])
        self.assertEqual(frames[1][2],struct.pack('<I',14<<24|4)+bytes(20)+b'test')

    def test_chunking_and_checksum_bytes_are_retained(self):
        image=bytearray(sample(dynamic=False,checksum=True))
        payload=bytes(range(251))*17
        struct.pack_into('<I',image,52,(2<<24)|(1<<28)|len(payload))
        image[64:]=payload+b'checksum'
        frames=list(packets(bytes(image)))
        self.assertEqual([len(x[2])-24 for x in frames[1:]],[2020,2020,235])
        self.assertEqual(b''.join(x[2][24:] for x in frames[1:]),payload+b'checksum')

    def test_padding_and_invalid_sizes(self):
        frame=usb_frame(bytes(488),14)
        self.assertEqual(len(frame),516)
        self.assertEqual(struct.unpack_from('<I',frame)[0],14<<24|492)
        for payload in (b'',bytes(0x4000)):
            with self.assertRaises(ValueError):usb_frame(payload,14)
        with self.assertRaises(ValueError):usb_frame(b'test',10)
        for seq in (-1,256,True):
            with self.assertRaises(ValueError):list(packets(sample(),seq))

    def test_plan_refuses_unhandled_security_and_requires_both_images(self):
        data=container([bb(1)])
        result=prepare(data,1,bytes.fromhex('ffffffff'))
        self.assertEqual([x['name'] for x in result['images']],['normal','bbmcu0'])
        self.assertFalse(result['executable'])
        for security in (b'',bytes(4),bytes.fromhex('fffffeff')):
            with self.assertRaises(ValueError):prepare(data,1,security)
        with self.assertRaises(ValueError):prepare(container([]),1,bytes.fromhex('ffffffff'))
