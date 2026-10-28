"""Synthetic host timing only; it cannot establish ESP32-C3 responsiveness."""
import ctypes as C
from pathlib import Path
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'model/tests'))
from test_recommend import Recipe, Result
with tempfile.TemporaryDirectory() as tmp:
    library = Path(tmp) / 'benchmark.so'
    subprocess.run(['cc', '-O2', '-shared', '-fPIC', str(ROOT/'model/kitchen_model.c'),
                    str(ROOT/'model/kitchen_recommend.c'), '-o', str(library)], check=True)
    fn = C.CDLL(str(library)).kitchen_recommend
    fn.argtypes = [C.POINTER(Recipe), C.c_size_t, C.c_uint64, C.c_uint, C.c_uint,
                   C.c_uint32, C.POINTER(C.c_uint16), C.c_size_t]
    fn.restype = Result
    for count in (30, 40):
        # All dishes fill all roles, repeat the same main and method, and
        # exceed the time limit, forcing all four relaxation levels.
        recipes = (Recipe*count)(*[Recipe(1, 1, 10, 100, 7, 1, 0) for _ in range(count)])
        start = time.perf_counter()
        result = fn(recipes, count, 0, 3, 1, 42, None, 0)
        elapsed = time.perf_counter()-start
        assert result.status == 0 and result.notes == 7
        print(f'{count} recipes: {elapsed:.3f} s (host, -O2, synthetic all-role case)')
