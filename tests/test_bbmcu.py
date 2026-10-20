import sys
import struct
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parents[1] / 'tools'))
from bbmcu import inventory
from test_firmware_elements import element
from test_firmware import sample


def container(elements):
    image=sample()
    data=bytearray(32)
    data[0:2]=b'\xff\x01'
    struct.pack_into('<4BII',data,16,1,1,0,0,32,len(image))
    data.extend(image)
    for e in elements:
        data.extend(bytes((-len(data))%16));data.extend(e)
    return bytes(data)


def bb(cv, ident=0, aid=0):
    return element(ident, sample(), bytes((cv,))+bytes(7), aid)


class BbMcuTests(unittest.TestCase):
    def test_revision_and_first_match_per_mcu(self):
        result=inventory(container([bb(2), bb(1), bb(0), bb(0,1)]),1)
        self.assertEqual([x['selected'] for x in result['bbmcu_candidates']],[False,True,False,True])
        self.assertEqual(result['selected_mcus'],[0,1])
        self.assertEqual(result['normal_firmware_candidate']['version'],'0.35.47.15')
        self.assertIsNone(result['resident_firmware_identity'])
        self.assertFalse(result['hardware_compatibility_verified'])

    def test_retains_file_order_and_does_not_invent_aid_filter(self):
        result=inventory(container([bb(0,aid=0x9999),bb(1)]),1)
        self.assertEqual([x['selected'] for x in result['bbmcu_candidates']],[True,False])
        self.assertEqual(result['bbmcu_candidates'][0]['aid_metadata'],0x9999)

    def test_no_suitable_candidate_and_invalid_input(self):
        self.assertEqual(inventory(container([bb(2)]),1)['selected_mcus'],[])
        for cv in (-1,256,True):
            with self.assertRaises(ValueError):inventory(container([]),cv)
        with self.assertRaises(ValueError):inventory(container([element(0,b'bad')]),1)
        with self.assertRaises(ValueError):inventory(container([bb(1)])[:-1],1)
