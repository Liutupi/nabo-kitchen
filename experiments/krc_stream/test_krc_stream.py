"""Standalone KRC1 streaming proof: synthetic recipes are not deliverable content."""
import binascii
import copy
import ctypes as ct
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SOURCE = Path(os.environ.get("NABO_RECIPE_ROOT", ROOT.parent / "nabo-kitchen"))
sys.path.insert(0, str(SOURCE / "tools"))
from check_recipes import load_recipes  # noqa: E402
from recipe_codec import canonical, pack_recipes, translate, unpack_recipes  # noqa: E402


class Pack(ct.Structure):
    _fields_ = [("data", ct.c_void_p), ("size", ct.c_size_t),
                *[(x, ct.c_uint32) for x in ("count", "dictionary_offset", "dictionary_length",
                                              "bitmap_offset", "bitmap_width", "body_offset")]]


class Cursor(ct.Structure):
    _fields_ = [("pack", Pack), ("next_id", ct.c_void_p), ("dictionary_end", ct.c_void_p),
                ("ordinal", ct.c_uint32), ("failed", ct.c_int)]


class Row(ct.Structure):
    _fields_ = [("ordinal", ct.c_uint32), ("id_hash", ct.c_uint32), ("tags", ct.c_uint16),
                ("courses", ct.c_uint8), ("minutes", ct.c_uint8),
                ("difficulty", ct.c_uint8), ("id", ct.c_char * 41)]


class Filter(ct.Structure):
    _fields_ = [("any_course", ct.c_uint8), ("main_ingredient", ct.c_int)]


class StreamTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        lib = Path(cls.tmp.name) / "stream.so"
        subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "main/kitchen"),
                        "-I", str(ROOT / "experiments/krc_stream"),
                        str(ROOT / "main/kitchen/kitchen_krc.c"),
                        str(ROOT / "experiments/krc_stream/krc_stream.c"), "-o", str(lib)],
                       check=True)
        cls.dll = ct.CDLL(str(lib))
        cls.dll.ksc_open.argtypes = [ct.POINTER(Cursor), ct.c_void_p, ct.c_size_t]
        cls.dll.ksc_next.argtypes = [ct.POINTER(Cursor), ct.POINTER(Row)]
        cls.dll.ksc_page.argtypes = [ct.POINTER(Cursor), Filter, ct.POINTER(Row), ct.c_size_t,
                                    ct.POINTER(ct.c_size_t), ct.POINTER(ct.c_int)]
        cls.dll.ksc_find_id.argtypes = [ct.c_void_p, ct.c_size_t, ct.c_char_p, ct.POINTER(Row)]
        cls.dll.ksc_body.argtypes = [ct.POINTER(Cursor), ct.c_uint32, ct.c_void_p, ct.c_size_t]
        cls.recipes = load_recipes(SOURCE)
        cls.actual = (ROOT / "assets/kitchen/kitchen_recipes.krc").read_bytes()
        cls.synthetic = {}
        for count in (1000, 1500):
            recipes = []
            for i in range(count):
                recipe = copy.deepcopy(cls.recipes[i % len(cls.recipes)])
                recipe["id"] = f"synthetic-{i:04d}"
                recipes.append(recipe)
            blob = pack_recipes(recipes, SOURCE)
            decoded, info = unpack_recipes(blob)
            cls.synthetic[count] = (blob, recipes, decoded, info)
            print(f"Synthetic {count}: KRC1 {len(blob)} B, dictionary {info['dictionary_bytes']} B")

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def opened(self, blob):
        buf = (ct.c_uint8 * len(blob)).from_buffer_copy(blob)
        cursor = Cursor()
        self.assertEqual(self.dll.ksc_open(ct.byref(cursor), buf, len(buf)), 1)
        return cursor, buf

    def pages(self, blob, course=0, main=-1, capacity=7):
        cursor, keep = self.opened(blob)
        rows, size, end = (Row * capacity)(), ct.c_size_t(), ct.c_int()
        result = []
        while not end.value:
            self.assertEqual(self.dll.ksc_page(ct.byref(cursor), Filter(course, main), rows,
                                               capacity, ct.byref(size), ct.byref(end)), 0)
            result.extend((rows[i].ordinal, rows[i].id.decode()) for i in range(size.value))
        self.assertEqual(cursor.ordinal, cursor.pack.count)
        return result

    def test_all_ids_and_pages_of_actual_and_synthetic_packs(self):
        for blob in [self.actual] + [self.synthetic[n][0] for n in (1000, 1500)]:
            decoded, info = unpack_recipes(blob)
            ids = [item["id"] for item in decoded]
            with self.subTest(count=len(ids)):
                self.assertEqual(self.pages(blob, capacity=1), list(enumerate(ids)))
                self.assertEqual(self.pages(blob, capacity=11), list(enumerate(ids)))
                self.assertEqual(self.pages(blob, capacity=64), list(enumerate(ids)))
                meat = [(i, ids[i]) for i, record in enumerate(info["records"])
                        if record["class_bits"] & 1]
                self.assertEqual(self.pages(blob, course=1), meat)
                ingredient = 0
                main = [(i, ids[i]) for i, record in enumerate(info["records"])
                        if blob[record["main_bitmap_offset"] + ingredient // 8] &
                        (1 << (ingredient % 8))]
                self.assertEqual(self.pages(blob, main=ingredient), main)

    def test_lookup_and_bounded_on_demand_body(self):
        for n in (1000, 1500):
            blob, recipes, _, info = self.synthetic[n]
            cursor, keep = self.opened(blob)
            for i in (0, 63, 64, 127, 128, 255, n // 2, n - 1):
                ident = f"synthetic-{i:04d}"
                row = Row()
                self.assertEqual(self.dll.ksc_find_id(keep, len(blob), ident.encode(), ct.byref(row)), 1)
                self.assertEqual((row.ordinal, row.id.decode()), (i, ident))
                buf = (ct.c_uint8 * 2048)()
                size = self.dll.ksc_body(ct.byref(cursor), i, buf, len(buf))
                expected = canonical(translate(recipes[i], info["dictionary"]))
                self.assertEqual(bytes(buf[:size]), expected)
                self.assertEqual(self.dll.ksc_body(ct.byref(cursor), i, buf, 1), -1)
            self.assertEqual(self.dll.ksc_find_id(keep, len(blob), b"synthetic-9999", ct.byref(Row())), 0)
            self.assertEqual(self.dll.ksc_body(ct.byref(cursor), n, buf, len(buf)), -1)

    def test_rejects_corruption_and_duplicate_stable_id(self):
        blob = self.synthetic[1000][0]
        for changed in (blob[:-1], blob[:100] + bytes([blob[100] ^ 1]) + blob[101:]):
            buf = (ct.c_uint8 * len(changed)).from_buffer_copy(changed)
            self.assertEqual(self.dll.ksc_open(ct.byref(Cursor()), buf, len(changed)), 0)
        # Even a resealed pack with duplicate IDs must not produce an ambiguous lookup.
        bad = bytearray(blob)
        off, length = struct.unpack_from("<II", bad, 20)
        needle, replacement = b'"synthetic-0999"', b'"synthetic-0000"'
        at = bad.index(needle, off, off + length)
        bad[at:at + len(needle)] = replacement
        bad[48 + 999 * 16:48 + 999 * 16 + 4] = bad[48:52]
        struct.pack_into("<I", bad, 44, binascii.crc32(bad[48:]))
        buf = (ct.c_uint8 * len(bad)).from_buffer_copy(bad)
        self.assertEqual(self.dll.ksc_find_id(buf, len(bad), b"synthetic-0000", ct.byref(Row())), -1)

    def test_caller_storage_does_not_scale_with_recipe_count(self):
        # Pointer size is host-dependent; this asserts fixed structures, not C3 availability.
        self.assertLessEqual(ct.sizeof(Cursor), 128)
        self.assertLessEqual(ct.sizeof(Row) * 8, 512)
        for n in (1000, 1500):
            self.assertEqual(self.opened(self.synthetic[n][0])[0].pack.count, n)
        print(f"Host fixed storage: cursor {ct.sizeof(Cursor)} B, eight rows {ct.sizeof(Row)*8} B")


if __name__ == "__main__":
    unittest.main(verbosity=2)
