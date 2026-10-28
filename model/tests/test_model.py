import ctypes as C
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'model/kitchen_model.c'

def load(directory, source):
    path = Path(directory) / 'candidate.c'; path.write_text(source)
    lib = Path(directory) / 'model.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',
                    '-I',str(ROOT/'model'),str(path),'-o',str(lib)],check=True)
    dll=C.CDLL(str(lib)); dll.kitchen_random_next.argtypes=[C.POINTER(C.c_uint32)]
    dll.kitchen_random_next.restype=C.c_uint32
    dll.kitchen_recipe_weight.argtypes=[C.c_uint,C.c_int,C.c_int,C.c_int,C.c_int]
    dll.kitchen_recipe_weight.restype=C.c_uint
    return dll

def replay(dll, trace):
    state=C.c_uint32()
    for item in trace['random']:
        if item['index']==0: state.value=item['seed']
        assert dll.kitchen_random_next(C.byref(state))==item['value'], item
    for item in trace['weights']:
        f=item['flags']
        assert dll.kitchen_recipe_weight(item['age'], f&1, f&2, f&4, f&8)==item['value'], item

class ModelParity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.trace=json.loads(subprocess.check_output(['node',str(ROOT/'model/tests/model_trace.mjs')]))
        cls.source=SOURCE.read_text()

    def test_reference(self):
        with tempfile.TemporaryDirectory() as tmp: replay(load(tmp,self.source),self.trace)

    def test_weight_mutation_is_detected(self):
        original='age < 7 ? 40'; self.assertEqual(self.source.count(original),1)
        with tempfile.TemporaryDirectory() as tmp, self.assertRaises(AssertionError):
            replay(load(tmp,self.source.replace(original,'age < 7 ? 50')),self.trace)

if __name__ == '__main__': unittest.main()
