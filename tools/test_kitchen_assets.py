"""Packing bounds, deterministic palettes/crops and integer pixel roundtrips."""
import binascii
import random
import struct
import unittest

import _bootstrap  # noqa: F401
from PIL import Image
from asset_codec import (ENTRY, GROUP, HEADER, OUTLINE, build_palette, index_image,
                         pack_sprites, packbits_decode, packbits_encode, prepare_pil,
                         rgb565, unpack_sprites)


def seal(data):
    values = list(HEADER.unpack_from(data))
    values[-1] = binascii.crc32(data[HEADER.size:])
    data[:HEADER.size] = HEADER.pack(*values)
    return bytes(data)


class AssetCodecTests(unittest.TestCase):
    def sample(self):
        image = Image.new('RGBA', (16, 12))
        for y in range(4, 9):
            for x in range(3, 11):
                image.putpixel((x, y), (255, 80, 30, 255))
        prepared = prepare_pil(image, image.size)
        return [{'id': 'tomato_test', 'group': 'dish', 'prepared': prepared}]

    def test_packbits_packets_and_seeded_random(self):
        rng = random.Random(9326)
        vectors = [b'', b'a', b'a'*2, b'a'*3, bytes(range(128)), b'a'*129,
                   b'abbccdddee' * 100, bytes(range(256))*3]
        vectors += [bytes(rng.randrange(64) for _ in range(n)) for n in range(0, 400, 13)]
        for raw in vectors:
            self.assertEqual(packbits_decode(packbits_encode(raw), len(raw)), raw)
        for encoded, expected in [(b'\x80', 1), (b'\x02x', 3), (b'\xffx', 127), (b'', 1)]:
            with self.assertRaises(ValueError):
                packbits_decode(encoded, expected)
        with self.assertRaises(ValueError):
            packbits_decode(b'', 1048577)

    def test_frame_crop_and_transparent_index_roundtrip(self):
        entries = self.sample()
        blob = pack_sprites(entries)
        pack = unpack_sprites(blob)
        asset = pack.assets['tomato_test']
        self.assertEqual(asset['frame'], (16, 12))
        self.assertEqual(asset['crop'], (3, 4, 8, 5))
        self.assertEqual(asset['indices'], index_image(entries[0]['prepared'], pack.groups['dish']))
        self.assertEqual(pack.image('tomato_test').getchannel('A').getbbox(), (3, 4, 11, 9))
        self.assertEqual(pack_sprites(entries), blob)
        self.assertLessEqual(len(pack.groups['dish']), 64)
        self.assertEqual(pack.groups['dish'][0:2], [0, rgb565(OUTLINE)])

    def test_blank_sprite_and_duplicate_ids(self):
        blank = prepare_pil(Image.new('RGBA', (4, 3)), (4, 3))
        entries = [{'id': 'blank', 'group': 'test', 'prepared': blank}]
        pack = unpack_sprites(pack_sprites(entries))
        self.assertEqual(pack.assets['blank']['crop'], (0, 0, 0, 0))
        self.assertEqual(pack.image('blank').getbbox(), None)
        with self.assertRaises(ValueError):
            pack_sprites(entries + entries)

    def test_half_alpha_matte_and_character_hair_not_canonicalized(self):
        image = Image.new('RGBA', (3, 3))
        image.putpixel((1, 1), (74, 46, 56, 255))
        food = prepare_pil(image, image.size)
        character = prepare_pil(image, image.size, character=True)
        self.assertEqual(food.rgb.getpixel((1, 1)), OUTLINE)
        self.assertEqual(character.rgb.getpixel((1, 1)), (74, 46, 56))
        self.assertEqual(character.stats['canonical_outline_pixels'], 0)
        image.putpixel((1, 1), (255, 255, 255, 128))
        prepared = prepare_pil(image, image.size)
        expected = tuple((255*128 + v*127 + 127)//255 for v in OUTLINE)
        self.assertEqual(prepared.rgb.getpixel((1, 1)), expected)
        self.assertEqual(prepared.mask.getpixel((1, 1)), 255)

    def test_shared_palette_determinism(self):
        images = [prepare_pil(Image.new('RGBA', (8, 8), color), (8, 8))
                  for color in [(200, 20, 30, 255), (20, 200, 30, 255)]]
        palette = build_palette(images)
        self.assertEqual(palette, build_palette(list(reversed(images))))
        self.assertTrue(all(0 < x < len(palette) for x in index_image(images[0], palette)))

    def test_corruption_checks_include_resealed_structural_bounds(self):
        blob = pack_sprites(self.sample())
        for cut in [0, 1, HEADER.size-1, len(blob)-1]:
            with self.assertRaises(ValueError):
                unpack_sprites(blob[:cut])
        broken = bytearray(blob);broken[-1] ^= 1
        with self.assertRaises(ValueError):
            unpack_sprites(broken)
        header = HEADER.unpack_from(blob)
        io, go = header[7], header[6]
        for field, invalid in [(2, 999), (3, 4), (4, 0), (6, 65535), (10, len(blob)+100), (11, len(blob))]:
            broken = bytearray(blob)
            record = list(ENTRY.unpack_from(broken, io));record[field] = invalid
            broken[io:io+ENTRY.size] = ENTRY.pack(*record)
            with self.assertRaises(ValueError):
                unpack_sprites(seal(broken))
        broken = bytearray(blob)
        record = list(GROUP.unpack_from(broken, go));record[1] = 65
        broken[go:go+GROUP.size] = GROUP.pack(*record)
        with self.assertRaises(ValueError):
            unpack_sprites(seal(broken))


if __name__ == '__main__':
    unittest.main()
