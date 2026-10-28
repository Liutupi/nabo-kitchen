"""KFN1 glyph masks, integer metrics and corrupt-input bounds."""
import binascii
import random
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from font_codec import (HEADER, FONT, GLYPH, Glyph, coverage_bytes, coverage_values,
                        pack_fonts, unpack_fonts)


def seal(blob):
    blob = bytearray(blob)
    values = list(HEADER.unpack_from(blob))
    values[-1] = binascii.crc32(blob[HEADER.size:])
    blob[:HEADER.size] = HEADER.pack(*values)
    return bytes(blob)


class FontCodecTests(unittest.TestCase):
    def sample(self, bpp=2):
        maximum = (1 << bpp) - 1
        return dict(id='han16', px=16, bpp=bpp, ascent=19, descent=5, line_height=24,
                    glyphs=[Glyph(32, 0, 0, 0, 0, 4, b''),
                            Glyph(65, 3, 3, -1, -8, 5, coverage_bytes([maximum, 0, maximum] * 3, bpp)),
                            Glyph(0x4e00, 32, 32, 0, -16, 16, coverage_bytes([maximum] * 1024, bpp))])

    def test_roundtrip_both_depths_and_empty_glyph(self):
        for bpp in (1, 2):
            font = self.sample(bpp)
            blob = pack_fonts([font])
            result = unpack_fonts(blob)
            for glyph in font['glyphs']:
                self.assertEqual(result.glyph('han16', glyph.cp), glyph)
            self.assertEqual(result.fonts['han16']['bpp'], bpp)
            self.assertEqual(blob, pack_fonts([font]))

    def test_raw_and_rle_paths(self):
        blob = pack_fonts([self.sample()])
        glyph_off = HEADER.unpack_from(blob)[6]
        codecs = {GLYPH.unpack_from(blob, glyph_off + n * GLYPH.size)[6] for n in range(3)}
        self.assertEqual(codecs, {0, 1})

    def test_invalid_encoder_and_coverage(self):
        for chars in ([1, 0, 2], [-1], [4]):
            with self.assertRaises(ValueError):
                coverage_bytes(chars, 1)
        with self.assertRaises(ValueError):
            coverage_values(b'\x01', 1, 2)
        for changes in ({'id': '../font'}, {'bpp': 3}, {'px': 65}):
            font = self.sample()
            font.update(changes)
            with self.assertRaises(ValueError):
                pack_fonts([font])
        with self.assertRaises(ValueError):
            pack_fonts([self.sample(), self.sample()])

    def test_truncation_crc_and_resealed_bounds(self):
        blob = pack_fonts([self.sample()])
        for cut in (0, 5, HEADER.size - 1, len(blob) - 1):
            with self.assertRaises(ValueError):
                unpack_fonts(blob[:cut])
        changed = bytearray(blob)
        changed[-1] ^= 1
        with self.assertRaises(ValueError):
            unpack_fonts(changed)
        glyph_off = HEADER.unpack_from(blob)[6]
        # Codepoint ordering, dimensions, codec and absolute payload bounds.
        for field, value in ((0, 0x110000), (1, 65), (6, 2), (7, 0), (8, 65535)):
            changed = bytearray(blob)
            at = glyph_off + GLYPH.size
            values = list(GLYPH.unpack_from(changed, at))
            values[field] = value
            changed[at:at + GLYPH.size] = GLYPH.pack(*values)
            with self.assertRaises(ValueError):
                unpack_fonts(seal(changed))

    def test_seeded_resealed_mutations_fail_cleanly(self):
        blob = pack_fonts([self.sample()])
        rng = random.Random(2841)
        for _ in range(300):
            changed = bytearray(blob)
            at = rng.randrange(HEADER.size, len(blob))
            changed[at] ^= 1 << rng.randrange(8)
            try:
                unpack_fonts(seal(changed))
            except ValueError:
                pass


if __name__ == '__main__':
    unittest.main()
