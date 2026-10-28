"""Offline fridge expands choices without changing ingredient identity."""
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from build_firmware_metadata import build, build_c


class FridgeMetadataTest(unittest.TestCase):
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
        self.assertIn(b'km_fridge_order_count = 50;', build_c(all_fridge=True))


if __name__ == '__main__':
    unittest.main()
