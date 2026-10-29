"""Production nibble CRC equals zlib for seeded, split and real asset inputs."""
import ctypes as ct
import os
from pathlib import Path
import random
import subprocess
import tempfile
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[1]


class CrcTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        path = Path(cls.temp.name)
        (path / 'crc.c').write_text('#include "kitchen_crc32.h"\n'
            'uint32_t crc(uint32_t seed,const uint8_t*p,size_t n){return kcrc32_update(seed,p,n);}\n')
        subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-Wall', '-Wextra', '-Werror',
                        '-shared', '-fPIC', '-I', str(ROOT/'main/kitchen'), str(path/'crc.c'),
                        '-o', str(path/'crc.so')], check=True)
        cls.lib = ct.CDLL(str(path/'crc.so'))
        cls.lib.crc.argtypes = [ct.c_uint32, ct.c_void_p, ct.c_size_t]
        cls.lib.crc.restype = ct.c_uint32

    @classmethod
    def tearDownClass(cls):
        if os.name == 'nt':
            from _ctypes import FreeLibrary
            FreeLibrary(cls.lib._handle)
        cls.temp.cleanup()

    def crc(self, data, seed=0):
        return self.lib.crc(seed, ct.create_string_buffer(data), len(data))

    def test_known_and_seeded_vectors(self):
        self.assertEqual(self.crc(b'123456789'), 0xcbf43926)
        rng = random.Random(291044)
        for size in [0, 1, 2, 15, 16, 31, 256, 2048, 4095, 4096, 4097, 65537]:
            raw = rng.randbytes(size)
            for seed in [0, 1, 0xffffffff, rng.getrandbits(32)]:
                self.assertEqual(self.crc(raw, seed), zlib.crc32(raw, seed))

    def test_real_bundle_and_chunked_updates(self):
        raw = (ROOT/'assets/kitchen/kitchen_bundle.bin').read_bytes()
        expected = zlib.crc32(raw)
        self.assertEqual(self.crc(raw), expected)
        for chunk in [1, 4096, 16384]:
            # Byte-at-a-time API exercised on the first4097bytes; full bundle
            # for production checksum chunk sizes.
            sample = raw[:4097] if chunk == 1 else raw
            crc = 0
            for at in range(0, len(sample), chunk):
                crc = self.crc(sample[at:at+chunk], crc)
            self.assertEqual(crc, zlib.crc32(sample))


if __name__ == '__main__':
    unittest.main(verbosity=2)
