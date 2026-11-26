from pathlib import Path
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'driverkit'))
from bind_firmware import bound_frames,generate
class DriverFirmwareBindTest(unittest.TestCase):
    def test_same_known_frames_as_existing_generator(self):
        data=(ROOT/'artifacts/rtw8922a_fw-1.bin').read_bytes()
        images=bound_frames(data)
        self.assertEqual(len(images),2)
        self.assertEqual(sum(len(frames) for _,frames in images),212)
        # The retained header is the prior boot_bundle output; compare raw bytes,
        # not another invocation of the new implementation.
        legacy=(ROOT/'artifacts/boot_payload.hpp').read_text().split('inline const std::vector')[0]
        old=[]
        for line in legacy.splitlines():
            if line.startswith('{') and line.endswith('},'):
                old.append(bytes(map(int,line[1:-2].split(','))))
        self.assertEqual([frame for _,frames in images for frame in frames],old)
    def test_modified_image_stops_before_output(self):
        source=bytearray((ROOT/'artifacts/rtw8922a_fw-1.bin').read_bytes());source[-1]^=1
        with tempfile.TemporaryDirectory() as temp:
            output=Path(temp)/'absent'
            with self.assertRaisesRegex(ValueError,'hash mismatch'):generate(bytes(source),output)
            self.assertFalse(output.exists())
if __name__=='__main__':unittest.main()
