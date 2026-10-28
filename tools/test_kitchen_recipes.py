"""KRC1 actual JSON roundtrips, 2KB cap, references and hostile binary bounds."""
import binascii
import copy
import struct
import unittest
from unittest.mock import patch

import _bootstrap  # noqa: F401
import heatshrink2
from check_recipes import ROOT, asset_rows, load_recipes, validate_recipe
from heatshrink_codec import decode, encode
from recipe_codec import HEADER, MAX_DECODED, PAYLOAD, pack_recipes, unpack_recipes


def seal(data):
    values = list(HEADER.unpack_from(data))
    values[-1] = binascii.crc32(data[HEADER.size:])
    data[:HEADER.size] = HEADER.pack(*values)
    return bytes(data)


class RecipeCodecTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Codec tests cover every current curated body; the CLI separately requires
        # physical PNGs, which may be in flight while a new menu is authored.
        cls.recipes = load_recipes(require_images=False)

    def test_all_real_curated_roundtrip_and_flash_index(self):
        blob = pack_recipes(self.recipes)
        decoded, info = unpack_recipes(blob)
        self.assertEqual(decoded, sorted(self.recipes, key=lambda x: x['id']))
        self.assertEqual(blob, pack_recipes(list(reversed(self.recipes))))
        self.assertEqual(info['index_record_bytes'], 16)
        self.assertTrue(all(r['decoded_bytes'] <= MAX_DECODED for r in info['records']))
        for recipe in decoded:
            self.assertEqual(validate_recipe(recipe, require_images=False), [])

    def test_heatshrink_independent_decoder_matches_c_binding(self):
        for raw in [b'hello hello hello', b'\0'*100, bytes(range(256))*3,
                    '番茄炒蛋，先热锅再下油。'.encode('utf-8')*15]:
            encoded = encode(raw)
            self.assertEqual(decode(encoded, len(raw)), raw)
            self.assertEqual(heatshrink2.decompress(encoded, window_sz2=8, lookahead_sz2=4), raw)
            with self.assertRaises(ValueError):
                decode(encoded + b'\0', len(raw))
        with self.assertRaises(ValueError):
            decode(b'', 2049)
        with self.assertRaises(ValueError):
            decode(b'\xff', 1)

    def test_vessel_sauce_garnish_and_png_foreign_keys(self):
        recipe = copy.deepcopy(self.recipes[0])
        assets = asset_rows()
        for ident in ['vessel_' + recipe['dish']['vessel'], 'vessel_' + recipe['dish']['vessel'] + '_shadow']:
            changed = dict(assets);changed.pop(ident)
            with patch('check_recipes.asset_rows', return_value=changed):
                self.assertTrue(any(ident in e for e in validate_recipe(recipe)))
        recipe['dish']['sauce'] = 'not_registered'
        self.assertTrue(any('sauce_not_registered' in e for e in validate_recipe(recipe)))
        recipe = copy.deepcopy(self.recipes[0]);recipe['dish']['garnish'] = ['shine']
        changed = dict(assets);changed.pop('garnish_shine')
        with patch('check_recipes.asset_rows', return_value=changed):
            self.assertTrue(any('garnish_shine' in e for e in validate_recipe(recipe)))
        with patch('pathlib.Path.is_file', return_value=False):
            self.assertTrue(any('missing dish PNG' in e for e in validate_recipe(self.recipes[0])))

    def test_numbered_payload_over_2kb_rejected(self):
        recipe = copy.deepcopy(self.recipes[0])
        recipe['steps'] = [{'text': '切'*48, 'tip': '轻'*16, 'method': 'cut', 'heat': 'off'} for _ in range(12)]
        self.assertEqual(validate_recipe(recipe), [])
        with self.assertRaisesRegex(ValueError, 'exceeds 2048'):
            pack_recipes([recipe])

    def test_corruption_crc_and_resealed_offsets_and_metadata(self):
        blob = pack_recipes(self.recipes)
        for cut in [0, 1, HEADER.size-1, len(blob)-1]:
            with self.assertRaises(ValueError):
                unpack_recipes(blob[:cut])
        broken = bytearray(blob);broken[-1] ^= 1
        with self.assertRaises(ValueError):
            unpack_recipes(broken)
        values = HEADER.unpack_from(blob);io, po = values[5], values[10]
        for offset, fmt, value in [(io+12, '<I', 0xffffffff), (io+7, '<B', 241),
                                   (po+2, '<H', 2049), (po, '<H', 65535)]:
            broken = bytearray(blob);struct.pack_into(fmt, broken, offset, value)
            with self.assertRaises(ValueError):
                unpack_recipes(seal(broken))
        broken = bytearray(blob);broken[io+9:io+12] = b'\xff'*3
        with self.assertRaises(ValueError):
            unpack_recipes(seal(broken))


if __name__ == '__main__':
    unittest.main()
