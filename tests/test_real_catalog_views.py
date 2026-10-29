"""Render every packaged recipe with actual metadata, fonts and sprite assets.

Set CJSON_DIR, or activate an ESP-IDF checkout providing components/json/cJSON.
This is a host check; timing and actual display behavior need device testing.
"""
import ctypes as ct
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets/kitchen"
NAMES = ("kitchen_recipes.krc", "kitchen_fonts.bin", "kitchen_dish.bin",
         "kitchen_icons.bin", "kitchen_ui.bin", "kitchen_nabo.bin")


class RealCatalogViews(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cjson = Path(os.environ.get("CJSON_DIR", ""))
        if not (cjson / "cJSON.c").is_file():
            idf = os.environ.get("IDF_PATH")
            if not idf:
                raise RuntimeError("Set CJSON_DIR or activate ESP-IDF")
            cjson = Path(idf) / "components/json/cJSON"
        if not (cjson / "cJSON.c").is_file():
            raise RuntimeError("cJSON source not found")
        cls.temp = tempfile.TemporaryDirectory()
        library = Path(cls.temp.name) / "view.so"
        sources = sorted((ROOT / "main/kitchen").glob("kitchen_*.c"))
        subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "main/kitchen"),
                        *(['-Wl,--export-all-symbols'] if os.name == 'nt' else []),
                        "-I", str(cjson), *map(str, sources),
                        str(ROOT / "tests/kitchen_view_host.c"), str(cjson / "cJSON.c"),
                        "-o", str(library)], check=True)
        cls.dll = ct.CDLL(str(library))
        cls.dll.view_open.argtypes = [x for _ in NAMES for x in (ct.POINTER(ct.c_uint8), ct.c_size_t)]
        for name in ("view_page", "view_actual_draw", "view_compare_hero_required",
                     "view_fridge_category"):
            getattr(cls.dll, name).argtypes = [ct.c_uint, ct.c_uint]
        for name in ("view_steps", "view_step", "view_step_scroll", "view_paper_scroll",
                     "view_book_boundary", "view_has_hero", "view_dish_hero_required"):
            getattr(cls.dll, name).argtypes = [ct.c_uint]
        cls.buffers = []
        args = []
        for name in NAMES:
            raw = (ASSETS / name).read_bytes()
            if name == NAMES[0]:
                offset, length = struct.unpack_from("<II", raw, 20)
                dictionary = json.loads(raw[offset:offset + length])
                cls.ids = dictionary["ids"]
                cls.ingredient_count = len(dictionary["ingredients"])
            buf = (ct.c_uint8 * len(raw)).from_buffer_copy(raw)
            cls.buffers.append(buf)
            args.extend((buf, len(buf)))
        if not cls.dll.view_open(*args):
            raise RuntimeError("Failed to open actual catalog and rendering assets")
        if cls.dll.view_count() != len(cls.ids):
            raise RuntimeError("Catalog count and KRC dictionary disagree")

    @classmethod
    def tearDownClass(cls):
        if os.name == 'nt':
            from _ctypes import FreeLibrary
            FreeLibrary(cls.dll._handle)
        cls.temp.cleanup()

    def check_render(self, where):
        self.assertEqual(self.dll.view_stripes_match(), 1, where)

    @staticmethod
    def scroll_positions(maximum):
        return sorted(set(range(0, maximum, 90)) | {maximum})

    def test_every_recipe_full_and_stripes(self):
        for index, ident in enumerate(self.ids):
            for page in (4, 5, 8):
                self.assertEqual(self.dll.view_page(page, index), 1, f"{ident}: page {page}")
                if page == 5:
                    for offset in self.scroll_positions(self.dll.view_paper_max()):
                        self.assertEqual(self.dll.view_paper_scroll(offset), 1, ident)
                        self.check_render(f"{ident}: ingredients offset {offset}")
                else:
                    self.check_render(f"{ident}: page {page}")
            steps = self.dll.view_steps(index)
            self.assertGreater(steps, 0, ident)
            for step in range(steps):
                self.assertEqual(self.dll.view_step(step), 1, f"{ident}: step {step}")
                for offset in self.scroll_positions(self.dll.view_step_max()):
                    self.assertEqual(self.dll.view_step_scroll(offset), 1, ident)
                    self.check_render(f"{ident}: step {step}, offset {offset}")

    def test_actual_book_entries_after_64(self):
        if len(self.ids) <= 64:
            self.skipTest("Current package does not exceed 64 recipes")
        for index in (64, len(self.ids) - 1):
            self.assertEqual(self.dll.view_book_boundary(index), 1, self.ids[index])

    def test_exclusion_limit_is_legible(self):
        self.assertEqual(self.dll.view_exclusion_settings(), 1)
        self.check_render("ingredient exclusion settings")

    def test_timer_controls_with_real_fonts(self):
        self.assertEqual(self.dll.view_page(7, 0), 1)
        self.check_render("timer page: cancel control and real glyphs")

    def test_opening_scene_waits_for_ok(self):
        self.assertEqual(self.dll.view_page(0, 0), 1)
        for elapsed in (0, 3599, 3600, 60000):
            self.assertEqual(self.dll.view_boot_page(elapsed), 0)
        self.assertEqual(self.dll.view_boot_at(60000), 1)
        self.check_render("opening scene after the animation")
        self.assertEqual(self.dll.view_boot_skip(), 1)

    def test_boot_and_home_keep_alert_above_the_content(self):
        pixels_type = ct.c_uint16 * (240 * 320)
        for page in (0, 1):
            self.assertEqual(self.dll.view_page(page, 0), 1)
            if page == 0:
                self.assertEqual(self.dll.view_boot_at(3500), 1)
            else:
                self.assertEqual(self.dll.view_scene_time(3500), 1)
            before = pixels_type()
            self.assertEqual(self.dll.view_draw(before, 0, 320), 1)
            self.assertEqual(self.dll.view_finished_alarm(), 1)
            after = pixels_type()
            self.assertEqual(self.dll.view_draw(after, 0, 320), 1)
            self.check_render(f"page {page}: alert")
            self.assertNotEqual(list(before[:240 * 30]), list(after[:240 * 30]))
            self.assertEqual(list(before[240 * 32:]), list(after[240 * 32:]),
                             f"page {page}: alarm covered the content")

    def test_real_catalog_draws(self):
        for combo in range(4):
            for seed in range(16):
                self.assertEqual(self.dll.view_actual_draw(seed, combo), 1,
                                 f"combo={combo}, seed={seed}")
    def test_cooperative_catalog_matches_every_item_and_yields_per_recipe(self):
        self.assertEqual(self.dll.view_catalog_cooperative(), 1)

    def test_fridge_categories_and_item_modes(self):
        for category in range(5):
            for mode in range(2):
                self.assertEqual(self.dll.view_fridge_category(category, mode), 1,
                                 f"category={category}, item_mode={mode}")
        self.assertEqual(self.dll.view_fridge_selection_badge(), 1)
        self.assertEqual(self.dll.view_fridge_every_ingredient(), self.ingredient_count)
        self.assertEqual(self.dll.view_fridge_fish_short_name(), 1)

    def test_hero_and_placeholder_compatibility(self):
        for ident in ("fanqie-chaodan", "qingzheng-luyu"):
            i = self.ids.index(ident)
            self.assertEqual(self.dll.view_dish_hero_required(i), 1, ident)
            self.assertEqual(self.dll.view_compare_hero_required(i, 1), 1, ident)
        # The release fixture has a complete illustration for every real recipe.
        # Placeholder/layer fallback remains covered by the synthetic view tests.
        for index, ident in enumerate(self.ids):
            self.assertEqual(self.dll.view_has_hero(index), 1, ident)
            self.assertEqual(self.dll.view_compare_hero_required(index, 1), 1, ident)


if __name__ == "__main__":
    unittest.main(verbosity=2)
