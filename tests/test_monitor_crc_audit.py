from pathlib import Path
import struct
import sys
import unittest
import zlib
sys.path.insert(0,str(Path(__file__).parents[1]/'tools'))
from monitor_crc_audit import audit


def capture(frame,flags=16,corrupt=False,truncated=False):
    radio=struct.pack('<BBHI',0,0,14,10)+bytes([flags,0])+struct.pack('<HH',2452,0)
    crc=zlib.crc32(frame)^(1 if corrupt else 0)
    packet=radio+frame+crc.to_bytes(4,'little')
    header=struct.pack('<IHHIIII',0xa1b2c3d4,2,4,0,0,2048,127)
    return header+struct.pack('<IIII',1,0,len(packet),len(packet)+int(truncated))+packet


class MonitorCrcTest(unittest.TestCase):
    def test_ack_and_cts_crc_not_just_flags(self):
        for fc,name in ((0xd4,'valid_ack'),(0xc4,'valid_cts')):
            frame=bytes([fc,0,0,0])+bytes.fromhex('0200008922a1')
            result=audit(capture(frame));self.assertEqual(result[name],1)
            self.assertEqual(result['valid_test_mac_frames'],1)
            bad=audit(capture(frame,corrupt=True));self.assertEqual(bad['crc_invalid'],1)
            self.assertEqual(bad['flag_crc_disagreements'],1);self.assertEqual(bad['valid_test_mac_frames'],0)

    def test_target_beacon_requires_crc_and_ssid(self):
        ap=bytes.fromhex('50926a110666')
        frame=bytes([0x80,0,0,0])+bytes([255])*6+ap+ap+bytes(2)+bytes(12)+b'\x00\x08Especial'
        self.assertEqual(audit(capture(frame))['valid_target_beacons'],1)
        self.assertEqual(audit(capture(frame,corrupt=True))['valid_target_beacons'],0)
        self.assertEqual(audit(capture(frame.replace(b'Especial',b'Otra-red')) )['valid_target_beacons'],0)

    def test_rejects_truncated_container_and_unsupported_link(self):
        data=capture(bytes([0xd4,0,0,0])+bytes(6))
        for invalid in (data[:10],data[:-1],data[:20]+bytes(4)+data[24:]):
            with self.assertRaises(ValueError):audit(invalid)
        self.assertEqual(audit(capture(bytes([0xd4,0,0,0])+bytes(6),truncated=True))['truncated_packets'],1)

    def test_bad_fcs_flag_disagreement_is_explicit(self):
        result=audit(capture(bytes([0xd4,0,0,0])+bytes(6),flags=80))
        self.assertEqual(result['crc_valid'],1);self.assertEqual(result['flag_crc_disagreements'],1)
