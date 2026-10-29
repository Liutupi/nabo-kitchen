"""Offline fridge expands choices without changing ingredient identity."""
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from build_firmware_metadata import build, build_c, build_retired
from asset_codec import fnv1a


class FridgeMetadataTest(unittest.TestCase):
    def test_retired_allowlist_matches_D2_and_excludes_current_ids(self):
        removed = json.loads((ROOT / 'data/D2_REMOVED.json').read_text(encoding='utf-8'))['removed']
        ids = {row['id'] for row in removed}
        self.assertEqual(len(ids), 35)
        self.assertFalse(ids & {p.stem for p in (ROOT / 'data/recipes/curated').glob('*.json')})
        header = build_retired().decode()
        for ident in ids:
            self.assertIn(f'case 0x{fnv1a(ident):08x}u:', header)
    def test_expanded_fridge_keeps_original_ordinal_and_order(self):
        default = json.loads(build())
        expanded = json.loads(build(all_fridge=True))
        self.assertEqual(default['ingredient_order'], expanded['ingredient_order'])
        self.assertEqual(default['ingredients'], expanded['ingredients'])
        self.assertEqual(len(default['fridge_order']), 30)
        self.assertEqual(expanded['fridge_order'], expanded['ingredient_order'])
        self.assertEqual(len(set(expanded['fridge_order'])), len(expanded['ingredients']))
        self.assertTrue({'meat', 'seafood', 'egg_tofu', 'veg', 'staple'} <=
                        {entry['category'] for entry in expanded['ingredients']})
        registry = json.loads((ROOT / 'data/ingredients.json').read_text(encoding='utf-8'))['ingredients']
        self.assertEqual(len(expanded['ingredients']), len(registry))
        self.assertIn(f'km_fridge_order_count = {len(registry)};'.encode(), build_c(all_fridge=True))


if __name__ == '__main__':
    unittest.main()
