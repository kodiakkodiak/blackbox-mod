"""Execute compiled ARM diagnostic and real stock packer; no hardware claims."""
import hashlib
import math
from pathlib import Path
import struct
import sys
import unittest
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT / 'diagnostics'))
from build import BASE, CAVE, SHA, patch
from emu import Emu, STACK

class DiagnosticTests(unittest.TestCase):
    def test_actual_converter_and_channel_isolation(self):
        e = Emu(str(ROOT / 'out/phones-diagnostic/BLACKBOX.bin'), str(ROOT / 'out/phones-diagnostic/diagnostic.elf'))
        a, b, c, dst, ref = 0x24010000, 0x24011000, 0x24012000, 0x24020000, 0x24022000
        for n in (0, 1, 31, 32, 128, 256):
            av = [0.2 * math.sin(i * .13) for i in range(n)]
            bv = [(i % 23 - 11) / 16 for i in range(n)]
            cv = [(i % 19 - 9) / 16 for i in range(n)]
            for at, values in ((a, av), (b, bv), (c, cv)):
                e.uc.mem_write(at, struct.pack(f'<{n}f', *values))
            inputs = [bytes(e.uc.mem_read(at, n * 4)) for at in (a,b,c)]
            for side, name in enumerate(('phones_diag_left', 'phones_diag_right')):
                sentinel = b'\xa5' * (16 * n + 32)
                e.uc.mem_write(dst, sentinel)
                e.uc.mem_write(ref, sentinel)
                e.uc.mem_write(STACK, struct.pack('<II', ref + side * 8, 4))
                e.call(0x0806002c, a, b, c, n)
                e.uc.mem_write(STACK, struct.pack('<II', dst + side * 8, 4))
                e.call(name, a, b, c, n)
                actual = bytes(e.uc.mem_read(dst, len(sentinel)))
                original = bytes(e.uc.mem_read(ref, len(sentinel)))
                for i in range(n):
                    at = i * 16 + side * 8
                    x, y = struct.unpack_from('<II', actual, at)
                    ox, oy = struct.unpack_from('<II', original, at)
                    self.assertEqual((x & 0xfff, y), (ox & 0xfff, oy), 'Out 1/3 converter fields changed')
                    v = x >> 12
                    if v & 0x80000:
                        v -= 0x100000
                    expected = int(.03125 * math.sin(2 * math.pi * (i % 32) * (side + 1) / 32) * 524288)
                    self.assertLessEqual(abs(v - expected), 1)
                    other = i * 16 + (1-side) * 8
                    self.assertEqual(actual[other:other+8], sentinel[other:other+8])
                self.assertEqual(actual[16*n:], sentinel[16*n:])
                self.assertEqual(inputs, [bytes(e.uc.mem_read(at,n*4)) for at in (a,b,c)])

    def test_patch_scope_and_preset_compatibility(self):
        stock = (ROOT / 'firmware/BLACKBOX-3.1.9.bin').read_bytes()
        result = (ROOT / 'out/phones-diagnostic/BLACKBOX.bin').read_bytes()
        self.assertEqual(hashlib.sha256(stock).hexdigest(), SHA)
        allowed = {at-BASE+i for at in (0x0804cf62,0x0804cf7c) for i in range(4)}
        changed = {i for i,(a,b) in enumerate(zip(stock,result)) if a != b}
        self.assertTrue(changed)
        self.assertLessEqual(changed, allowed)
        # This diagnostic adds no settings and leaves ALL stock preset code/data intact.
        self.assertLessEqual(len(result), 0xc0000)

    def test_reject_nonstock(self):
        with self.assertRaises(ValueError):
            patch(b'not stock', b'test', {})
        stock = (ROOT / 'firmware/BLACKBOX-3.1.9.bin').read_bytes()
        with self.assertRaises(ValueError):
            patch(stock, b'x' * (0x08100000-CAVE+1), {})

if __name__ == '__main__':
    unittest.main()
