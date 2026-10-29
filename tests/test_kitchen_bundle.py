"""Production C offline asset bundle: exact payloads and malformed index rejection."""
import ctypes
import hashlib
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[1]
FILES = ('kitchen_recipes.krc', 'kitchen_fonts.bin', 'kitchen_dish.bin',
         'kitchen_icons.bin', 'kitchen_ui.bin', 'kitchen_nabo.bin')

class Blob(ctypes.Structure):
    _fields_ = [('data', ctypes.c_void_p), ('size', ctypes.c_size_t)]
class Assets(ctypes.Structure):
    _fields_ = [(name, Blob) for name in ('recipes', 'fonts', 'dish', 'icons', 'ui', 'animation')]

class BundleTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        dll = Path(cls.temp.name)/'bundle.so'
        subprocess.run([os.environ.get('CC','cc'), '-std=c11', '-Wall', '-Wextra', '-Werror',
                        '-shared', '-fPIC', '-I', str(ROOT/'main/kitchen'),
                        str(ROOT/'main/kitchen/kitchen_bundle.c'), '-o', str(dll)], check=True)
        cls.lib = ctypes.CDLL(str(dll))
        cls.lib.kb_open.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(Assets)]
        cls.lib.kb_open.restype = ctypes.c_bool
        cls.progress_type = ctypes.CFUNCTYPE(None, ctypes.c_void_p)
        cls.lib.kb_open_cooperative.argtypes = [ctypes.c_void_p, ctypes.c_size_t,
                                               ctypes.POINTER(Assets), cls.progress_type,
                                               ctypes.c_void_p]
        cls.lib.kb_open_cooperative.restype = ctypes.c_bool
        cls.bundle = (ROOT/'assets/kitchen/kitchen_bundle.bin').read_bytes()
    @classmethod
    def tearDownClass(cls):
        if os.name == 'nt':
            from _ctypes import FreeLibrary
            FreeLibrary(cls.lib._handle)
        cls.temp.cleanup()
    def open(self, raw):
        buf = ctypes.create_string_buffer(bytes(raw))
        result = Assets()
        ok = self.lib.kb_open(buf, len(raw), ctypes.byref(result))
        return ok, result, buf
    @staticmethod
    def reseal(raw):
        struct.pack_into('<I', raw, 12, zlib.crc32(raw[16:]))
        return raw
    def test_complete_and_exact_resources(self):
        ok, result, buf = self.open(self.bundle)
        self.assertTrue(ok)
        for (name, _), file in zip(Assets._fields_, FILES):
            item = getattr(result, name)
            actual = ctypes.string_at(item.data, item.size)
            self.assertEqual(actual, (ROOT/'assets/kitchen'/file).read_bytes())
    def test_reject_corruption_and_truncation(self):
        bad = bytearray(self.bundle);bad[-1] ^= 1
        self.assertFalse(self.open(bad)[0])
        for length in (0,15,111,len(self.bundle)-1):
            self.assertFalse(self.open(self.bundle[:length])[0])
    def test_cooperative_validation_has_bounded_checksum_slices(self):
        calls = []
        marker = 123
        cb = self.progress_type(lambda context: calls.append(context))
        buf = ctypes.create_string_buffer(self.bundle)
        out = Assets()
        self.assertTrue(self.lib.kb_open_cooperative(buf, len(self.bundle), ctypes.byref(out), cb, marker))
        sizes = [len(self.bundle) - 16]
        sizes += [struct.unpack_from('<I', self.bundle, 24 + i * 16)[0] for i in range(6)]
        self.assertEqual(calls, [marker] * sum((n + 4095) // 4096 for n in sizes))
        for (name, _), file in zip(Assets._fields_, FILES):
            item = getattr(out, name)
            self.assertEqual(ctypes.string_at(item.data, item.size), (ROOT/'assets/kitchen'/file).read_bytes())
        bad = bytearray(self.bundle); bad[-1] ^= 1
        buf = ctypes.create_string_buffer(bytes(bad))
        self.assertFalse(self.lib.kb_open_cooperative(buf, len(bad), ctypes.byref(out), cb, marker))
        self.assertFalse(out.recipes.data)
    def test_reject_resealed_invalid_entries(self):
        mutations = ((16, '<I', 2),       # wrong or duplicate id
                     (20, '<I', 0),       # offset before table
                     (20, '<I', 113),     # unaligned offset
                     (24, '<I', 0),       # empty payload
                     (24, '<I', 0xffffffff),
                     (36, '<I', 112),     # overlap with first payload
                     (28, '<I', 0))       # invalid payload checksum
        for offset, fmt, value in mutations:
            with self.subTest(offset=offset,value=value):
                bad = bytearray(self.bundle)
                struct.pack_into(fmt,bad,offset,value)
                self.assertFalse(self.open(self.reseal(bad))[0])
    def test_deterministic_rebuild(self):
        with tempfile.TemporaryDirectory() as folder:
            out=Path(folder)/'bundle.bin'
            subprocess.run(['python3',str(ROOT/'tools/pack_kitchen_assets.py'),'--asset-dir',str(ROOT/'assets/kitchen'),'--output',str(out)],check=True,capture_output=True)
            self.assertEqual(hashlib.sha256(out.read_bytes()).digest(),hashlib.sha256(self.bundle).digest())

if __name__ == '__main__':unittest.main()
