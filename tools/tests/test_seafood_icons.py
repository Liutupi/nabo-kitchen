"""Reproducible source artwork and palette isolation for new seafood icons."""
import csv
import hashlib
import json
from pathlib import Path
import sys
import unittest

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
import _bootstrap  # noqa: F401
from asset_codec import pack_sprites, prepare_image, unpack_sprites
from generate_seafood_icons import ITEMS, draw_icon
from pack_assets import profile


class SeafoodIconsTest(unittest.TestCase):
    def test_sources_and_review_metadata(self):
        with (ROOT / 'game-art/kitchen/ASSET_LIST.csv').open(encoding='utf-8-sig', newline='') as f:
            rows = {row['id']: row for row in csv.DictReader(f)}
        hashes = set()
        for ident, subject in ITEMS.items():
            path = ROOT / 'game-art/kitchen/ingredients' / f'{ident}.png'
            original = path.parent / 'originals' / f'{ident}-source.png'
            meta = json.loads(path.with_suffix('.json').read_text(encoding='utf-8'))
            row = rows[ident]
            self.assertEqual((row['category'], row['status']), ('ingredient', 'review'))
            self.assertEqual(meta['subject'], subject)
            self.assertEqual(meta['sha256'], hashlib.sha256(path.read_bytes()).hexdigest())
            self.assertEqual(meta['original_sha256'], hashlib.sha256(original.read_bytes()).hexdigest())
            with Image.open(original) as source, Image.open(path) as output:
                self.assertEqual(source.size, (640, 640))
                self.assertEqual(output.size, (160, 160))
                self.assertEqual(source.convert('RGBA').tobytes(), draw_icon(ident).tobytes())
                self.assertEqual(output.convert('RGBA').tobytes(),
                                 draw_icon(ident).resize((160, 160), Image.Resampling.LANCZOS).tobytes())
                left, top, right, bottom = output.getchannel('A').getbbox()
                self.assertGreaterEqual(min(left, top, 160-right, 160-bottom), 12)
                self.assertEqual(output.getchannel('A').getextrema(), (0, 255))
                hashes.add(hashlib.sha256(output.tobytes()).hexdigest())
        self.assertEqual(len(hashes), len(ITEMS))

    def test_new_palettes_preserve_old_ingredient_pixels(self):
        with (ROOT / 'game-art/kitchen/ASSET_LIST.csv').open(encoding='utf-8-sig', newline='') as f:
            rows = [row for row in csv.DictReader(f) if row['category'] == 'ingredient']
        entries = []
        for row in rows:
            pool, group, size, character, key = profile(row)
            self.assertEqual(pool, 'icons')
            entries.append({'id': row['id'], 'group': group,
                            'prepared': prepare_image(ROOT / 'game-art/kitchen' / row['path'],
                                                      size, character, key)})
        old_entries = [entry for entry in entries if entry['id'] not in ITEMS]
        old = unpack_sprites(pack_sprites(old_entries))
        new = unpack_sprites(pack_sprites(entries))
        for entry in old_entries:
            ident = entry['id']
            self.assertEqual(old.image(ident).tobytes(), new.image(ident).tobytes(), ident)
        self.assertEqual(len(new.assets) - len(old.assets), 8)


if __name__ == '__main__':
    unittest.main()
