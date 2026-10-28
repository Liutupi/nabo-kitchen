"""Optional dish heroes must preserve old recipe encoding and layering fallback."""
import tempfile
import unittest
from pathlib import Path
import hashlib
import json
import sys
from unittest.mock import patch

from PIL import Image, ImageDraw
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _bootstrap  # noqa: F401
from asset_codec import pack_sprites, prepare_pil, unpack_sprites
from check_recipes import ROOT, asset_rows, dish_asset_ids, load_recipes
from pack_assets import build, composite_recipes, profile
from recipe_codec import pack_recipes


class HeroAssets(unittest.TestCase):
    def test_registered_sample_preserves_original_and_review_status(self):
        rows = asset_rows(ROOT)
        base = ROOT / 'game-art/kitchen/dish-hero'
        heroes = [ident for ident, row in rows.items() if row['category'] == 'hero']
        self.assertGreaterEqual(len(heroes), 17)
        for ident in heroes:
            with self.subTest(ident=ident):
                self.assertIn(rows[ident]['status'], ('review', 'done'))
                metadata = json.loads((base / (ident + '.json')).read_text(encoding='utf-8'))
                self.assertTrue((ROOT / 'data/recipes/curated' /
                                 (metadata['recipe_id'] + '.json')).is_file())
                self.assertEqual(ident, 'hero_' + metadata['recipe_id'])
                original = base / metadata['original']
                source = base / (ident + '.png')
                self.assertEqual(metadata['status'], rows[ident]['status'])
                self.assertEqual(hashlib.sha256(original.read_bytes()).hexdigest(), metadata['original_sha256'])
                self.assertEqual(hashlib.sha256(source.read_bytes()).hexdigest(), metadata['asset_sha256'])
                with Image.open(source) as image:
                    self.assertEqual(image.size, (640, 480))
                    self.assertEqual(image.mode, 'RGBA')
                    self.assertEqual(image.getchannel('A').getextrema(), (0, 255))

    def test_profile_rejects_unsafe_names_and_routes_to_own_palette(self):
        row = {'id': 'hero_fanqie-chaodan', 'category': 'hero',
               'path': 'dish-hero/hero_fanqie-chaodan.png'}
        self.assertEqual(profile(row), ('dish', row['id'], (160, 120), False, False))
        for ident, path in [('hero_../x', 'dish-hero/hero_../x.png'),
                            ('hero_' + 'x' * 41, 'dish-hero/hero_' + 'x' * 41 + '.png'),
                            ('hero_egg', '../hero_egg.png')]:
            with self.subTest(ident=ident), self.assertRaises(ValueError):
                profile({**row, 'id': ident, 'path': path})

    def test_build_transparent_hero_and_reject_bad_alpha_or_dimensions(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            base = root / 'game-art/kitchen'
            (base / 'dish-hero').mkdir(parents=True)
            (base / 'ASSET_LIST.csv').write_text('id,category,path\n')
            ident = 'hero_fanqie-chaodan'
            source = base / 'dish-hero' / (ident + '.png')
            row = {'id': ident, 'category': 'hero', 'path': 'dish-hero/' + ident + '.png',
                   'source': '640x480'}
            image = Image.new('RGBA', (640, 480))
            ImageDraw.Draw(image).ellipse((100, 80, 500, 400), fill=(230, 50, 30, 255))
            image.save(source)
            with patch('pack_assets.asset_rows', return_value={ident: row}):
                report, decoded = build(root, root / 'out', decode_pngs=False)
                pack = decoded['dish']
                self.assertEqual(pack.assets[ident]['frame'], (160, 120))
                self.assertEqual(pack.assets[ident]['group'], ident)
                self.assertEqual(pack.image(ident).getpixel((0, 0))[3], 0)
                self.assertGreater(pack.image(ident).getpixel((75, 60))[3], 0)
                self.assertTrue(report['roundtrip'])
                Image.new('RGB', (640, 480), 'white').save(source)
                with self.assertRaisesRegex(ValueError, 'transparent alpha'):
                    build(root, root / 'out', decode_pngs=False)
                Image.new('RGBA', (640, 480)).save(source)
                with self.assertRaisesRegex(ValueError, 'fully transparent'):
                    build(root, root / 'out', decode_pngs=False)
                Image.new('RGBA', (100, 100)).save(source)
                with self.assertRaisesRegex(ValueError, '640x480'):
                    build(root, root / 'out', decode_pngs=False)

    def test_composite_prefers_hero_and_falls_back_for_other_recipes(self):
        recipes = load_recipes()[:2]
        entries = []
        needed = set(dish_asset_ids(recipes[1]['dish']))
        for ident in needed:
            img = Image.new('RGBA', (160, 120), (0, 0, 0, 0))
            ImageDraw.Draw(img).rectangle((5, 5, 30, 30), fill=(30, 180, 60, 255))
            entries.append({'id': ident, 'group': 'fallback', 'prepared': prepare_pil(img, img.size)})
        hero = 'hero_' + recipes[0]['id']
        img = Image.new('RGBA', (160, 120))
        ImageDraw.Draw(img).rectangle((50, 40, 110, 80), fill=(240, 80, 20, 255))
        entries.append({'id': hero, 'group': hero, 'prepared': prepare_pil(img, img.size)})
        pack = unpack_sprites(pack_sprites(entries))
        with tempfile.TemporaryDirectory() as folder:
            records = composite_recipes(pack, recipes, folder)
            self.assertEqual(records[0]['layers'], [hero])
            self.assertEqual(records[1]['layers'], dish_asset_ids(recipes[1]['dish']))
            self.assertEqual(Image.open(Path(folder) / (recipes[0]['id'] + '.png')).tobytes(),
                             pack.image(hero).tobytes())

    def test_hero_registration_does_not_change_krc_bytes(self):
        recipes = load_recipes()[:2]
        original = asset_rows(ROOT)
        with patch('recipe_codec.asset_rows', return_value=original):
            before = pack_recipes(recipes)
        hero = {'id': 'hero_' + recipes[0]['id'], 'category': 'hero',
                'path': 'dish-hero/hero_' + recipes[0]['id'] + '.png'}
        with patch('recipe_codec.asset_rows', return_value={**original, hero['id']: hero}):
            self.assertEqual(before, pack_recipes(recipes))


if __name__ == '__main__':
    unittest.main()
