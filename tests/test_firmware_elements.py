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
        chosen=r['power_candidates']
        self.assertEqual(len(chosen),1)
        self.assertEqual(chosen[0]['rfe_type'],1)
        self.assertEqual(chosen[0]['aid'],0)

    def test_bad_directory_and_power_length(self):
        with self.assertRaises(ValueError):m.inspect(b'bad')
        with self.assertRaises(ValueError):m.inspect(b'\xff\xff'+b'\0'*14)
        with self.assertRaises(ValueError):m.inspect(container([element(9,b'123',b'\0\0\1\4\1\0\0\0')]))

    def test_physical_aid_and_radio_slot_selection(self):
        pairs=struct.pack('<II',1,2)
        raw=container([element(2,pairs,aid=0x1111),element(2,pairs,aid=0x6933),
                       element(2,pairs,aid=0),
                       element(4,pairs,b'\1'+b'\0'*7),
                       element(5,pairs,b'\1'+b'\0'*7),
                       power(1,0x1111),power(1,0x6933),power(0)])
        r=m.inspect(raw,aid=0x6933)
        self.assertTrue(r['actual_chip_aid_known'])
        self.assertEqual(r['aid_selection_scope'],'generic_and_matching')
        self.assertEqual(r['register_candidates'][0]['aid'],0x6933)
        self.assertEqual(len(r['register_candidates']),2)
        self.assertEqual(r['register_candidates'][1]['id'],4)
        self.assertEqual(r['register_candidates'][1]['target'],'rf_radio[1]')
        self.assertEqual(r['power_candidates'][0]['aid'],0x6933)

    def test_invalid_identity_and_radio_index(self):
        for aid in (-1,65536):
            with self.assertRaises(ValueError):m.inspect(container([]),aid=aid)
        with self.assertRaises(ValueError):m.inspect(container([]),rfe=256)
        with self.assertRaises(ValueError):m.inspect(container([
            element(4,b'12345678',b'\4'+b'\0'*7)]))
