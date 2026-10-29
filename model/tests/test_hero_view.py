"""Real C renderer chooses a complete transparent hero or the old layer fallback."""
import ctypes as C
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from PIL import Image, ImageDraw
from package_fixtures import FROZEN, FROZEN_META, recipe_ids

ROOT = Path(__file__).resolve().parents[2]
PKG = ROOT / 'checkpoints/2026-10-28/packages'
sys.path.insert(0, str(ROOT / 'tools'))
from asset_codec import pack_sprites, prepare_pil, rgb565, unpack_sprites


class HeroView(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cjson = os.environ.get('CJSON_DIR')
        if not cjson or not (Path(cjson) / 'cJSON.c').is_file():
            raise unittest.SkipTest('Set CJSON_DIR to pinned ESP-IDF cJSON')
        cls.tmp = tempfile.TemporaryDirectory()
        library = Path(cls.tmp.name) / 'view.so'
        sources = [p for p in sorted((ROOT / 'model').glob('kitchen_*.c'))
                   if p.name != 'kitchen_metadata.c'] + [FROZEN_META]
        subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-shared',
                        '-fPIC', '-I', str(ROOT / 'model'), '-I', cjson,
                        *map(str, sources), str(ROOT / 'model/tests/view_host.c'),
                        str(Path(cjson) / 'cJSON.c'), '-o', str(library)], check=True)
        cls.dll = C.CDLL(str(library))
        cls.dll.view_open.argtypes = [x for _ in range(6) for x in (C.POINTER(C.c_uint8), C.c_size_t)]
        cls.dll.view_page.argtypes = [C.c_uint, C.c_uint]
        cls.dll.view_food.argtypes = [C.POINTER(C.c_uint16), C.c_int, C.c_int, C.c_uint]
        cls.ids = recipe_ids(FROZEN)
        src = unpack_sprites((PKG / 'kitchen_dish.bin').read_bytes())
        entries = [{'id': ident, 'group': item['group'],
                    'prepared': prepare_pil(src.image(ident), (160, 120))}
                   for ident, item in src.assets.items()]
        cls.fallback = pack_sprites(entries)
        image = Image.new('RGBA', (160, 120))
        ImageDraw.Draw(image).ellipse((40, 20, 120, 100), fill=(225, 40, 30, 255))
        cls.hero_id = 'hero_' + cls.ids[0]
        cls.with_hero = pack_sprites(entries + [{'id': cls.hero_id, 'group': cls.hero_id,
                                                 'prepared': prepare_pil(image, image.size)}])
        cls.expected = unpack_sprites(cls.with_hero).image(cls.hero_id)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def open(self, dish):
        paths = [FROZEN, PKG / 'kitchen_fonts.bin', None, PKG / 'kitchen_icons.bin',
                 PKG / 'kitchen_ui.bin', PKG / 'kitchen_nabo.bin']
        self.buffers = []
        args = []
        for path in paths:
            raw = dish if path is None else path.read_bytes()
            buf = (C.c_uint8 * len(raw)).from_buffer_copy(raw)
            self.buffers.append(buf)
            args.extend((buf, len(buf)))
        self.assertEqual(self.dll.view_open(*args), 1)
        self.assertEqual(self.dll.view_page(4, 0), 1)

    def food(self, index, stripes=False):
        result = []
        for y in range(0, 120, 8 if stripes else 120):
            height = 8 if stripes else 120
            buf = (C.c_uint16 * (160 * height))()
            self.assertEqual(self.dll.view_food(buf, y, height, index), 1)
            result.extend(buf)
        return result

    def test_hero_priority_alpha_stripes_and_other_recipe_fallback(self):
        self.open(self.fallback)
        first_fallback, second_fallback = self.food(0), self.food(1)
        self.open(self.with_hero)
        hero = self.food(0)
        self.assertNotEqual(hero, first_fallback)
        self.assertEqual(hero, self.food(0, stripes=True))
        self.assertEqual(self.food(1), second_fallback)
        expected = [0x18c6 if a == 0 else rgb565((r, g, b))
                    for r, g, b, a in self.expected.get_flattened_data()]
        self.assertEqual(hero, expected, 'hero must replace rather than overlay fallback')


if __name__ == '__main__':
    unittest.main()
