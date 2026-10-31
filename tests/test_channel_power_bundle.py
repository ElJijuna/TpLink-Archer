"""Integration checks against the board-bound firmware; skipped without local inputs."""
import hashlib
import re
import runpy
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))


@unittest.skipUnless((ROOT / 'artifacts/efuse-stage13/phycap.bin').exists(),
                     'Requires captured board inputs')
class ChannelPowerBundleTests(unittest.TestCase):
    def test_channel9_uses_its_firmware_cells_and_preserves_channel1(self):
        data = runpy.run_path(str(ROOT / 'tools/channel_bundle.py'))
        header = (ROOT / 'artifacts/channel_payload.hpp').read_text()
        old, new = header.split('inline const std::vector<archer::Channel1BbWrite> trusted_channel9_world_power')
        self.assertEqual(hashlib.sha256(old.encode()).hexdigest(),
                         '579c24d2a27b3711c2843212abd71bfb43f260eb8ceff19267c5e6dbb108e6d0')
        writes = [(int(a), int(m), int(v)) for a, m, v in
                  re.findall(r'\{(\d+),(\d+)u,(\d+)u\}', new)]
        self.assertEqual(len(writes), 197)
        words = {a: v for a, m, v in writes}
        def byte(addr, offset):
            return (words[addr + (offset // 4) * 4] >> ((offset % 4) * 8)) & 255
        # Independently read raw selected firmware entries, with channel9 index8.
        limit = {tuple(e[:6]): int.from_bytes(e[6:7], 'little', signed=True)
                 for e in data['entries'](10, 7)}
        ru = {tuple(e[:4]): int.from_bytes(e[4:5], 'little', signed=True)
              for e in data['entries'](13, 5)}
        for ntx in range(2):
            for pair, (bw, rs) in enumerate(((0, 0), (1, 0), (0, 1), (0, 2))):
                for bf in range(2):
                    expected = min(limit.get((bw, ntx, rs, bf, 0, 8), 0) // 2, 63)
                    self.assertEqual(byte(0x11fac + ntx * 76, pair * 2 + bf), expected & 255)
            for r in range(5):
                expected = min(ru.get((r, ntx, 0, 8), 0) // 2, 63)
                self.assertEqual(byte(0x12048 + ntx * 80, r * 16), expected & 255)
