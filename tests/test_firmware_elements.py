import importlib.util
import struct
import unittest
spec=importlib.util.spec_from_file_location('firmware_elements','tools/firmware_elements.py')
m=importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


def element(ident,payload,private=b'\0'*8,aid=0):
    h=bytearray(32)
    struct.pack_into('<II',h,0,ident,len(payload))
    struct.pack_into('<H',h,12,aid)
    h[24:32]=private
    return bytes(h)+payload


def container(elements):
    data=bytearray(48)
    data[0:2]=b'\xff\x01'
    struct.pack_into('<4BII',data,16,1,1,0,0,32,16)
    for e in elements:
        data.extend(b'\0'*((-len(data))%16));data.extend(e)
    return bytes(data)


def power(rfe,aid=0):
    return element(9,b'1234',b'\0\0'+bytes((rfe,4))+struct.pack('<I',1),aid)


class ElementTests(unittest.TestCase):
    def test_register_pair_and_bounds(self):
        raw=container([element(2,struct.pack('<II',0x1234,0x5678))])
        r=m.inspect(raw)
        self.assertEqual(r['elements'][0]['register_pairs'],1)
        self.assertFalse(r['hardware_compatibility_validated'])
        with self.assertRaises(ValueError):m.inspect(raw[:-1])
        with self.assertRaises(ValueError):m.inspect(container([element(2,b'bad')]))
        bad=bytearray(raw);struct.pack_into('<I',bad,20,16)
        with self.assertRaises(ValueError):m.inspect(bad)

    def test_rfe_preference_generic_aid_only(self):
        r=m.inspect(container([power(0),power(1),power(0),power(2),power(1,0x1234)]))
        chosen=r['generic_aid_power_candidates']
        self.assertEqual(len(chosen),1)
        self.assertEqual(chosen[0]['rfe_type'],1)
        self.assertEqual(chosen[0]['aid'],0)

    def test_bad_directory_and_power_length(self):
        with self.assertRaises(ValueError):m.inspect(b'bad')
        with self.assertRaises(ValueError):m.inspect(b'\xff\xff'+b'\0'*14)
        with self.assertRaises(ValueError):m.inspect(container([element(9,b'123',b'\0\0\1\4\1\0\0\0')]))
