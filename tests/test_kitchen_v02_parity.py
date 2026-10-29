"""Replay JavaScript prototype vectors against the actual C implementation."""
import ctypes as C
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
class Recipe(C.Structure):
    _fields_=[('ingredients',C.c_uint64),('main',C.c_uint64),('minutes',C.c_uint16),('weight',C.c_uint16),('courses',C.c_uint8),('method',C.c_uint8),('repeated',C.c_uint8)]
class Result(C.Structure):
    _fields_=[('status',C.c_int),('indices',C.c_uint16*4),('count',C.c_uint8),('notes',C.c_uint8)]
class V02Parity(unittest.TestCase):
    def test_javascript_vectors_against_production_C(self):
        data=json.loads((ROOT/'tests/fixtures/kitchen_v02_parity.json').read_text(encoding='utf-8'))
        with tempfile.TemporaryDirectory() as tmp:
            lib=Path(tmp)/'v02.so'
            subprocess.run([os.environ.get('CC','cc'),'-std=c11','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-I',str(ROOT/'main/kitchen'),str(ROOT/'main/kitchen/kitchen_model.c'),str(ROOT/'main/kitchen/kitchen_recommend.c'),'-o',str(lib)],check=True)
            dll=C.CDLL(str(lib))
            try:
                dll.kitchen_recipe_weight.argtypes=[C.c_uint,C.c_int,C.c_int,C.c_int,C.c_int]
                for row in data['weights']:
                    f=row['flags'];self.assertEqual(dll.kitchen_recipe_weight(row['age'],f&1,f&2,f&4,f&8),row['value'],row)
                ingredients=sorted({x for r in data['recipes'] for x in r['ingredients']})
                methods=sorted({r['method'] for r in data['recipes']})
                self.assertLessEqual(len(ingredients),64)
                bits=lambda ids:sum(1<<ingredients.index(x) for x in set(ids))
                recipes=(Recipe*len(data['recipes']))(*[Recipe(bits(r['ingredients']),bits(r['main']),r['minutes'],r['weight'],sum({'meat':1,'veg':2,'soup':4}.get(x,0) for x in r['courses']),methods.index(r['method']),len(r['main'])!=len(set(r['main']))) for r in data['recipes']])
                dll.kitchen_recommend.argtypes=[C.POINTER(Recipe),C.c_size_t,C.c_uint64,C.c_uint,C.c_uint,C.c_uint32,C.POINTER(C.c_uint16),C.c_size_t]
                dll.kitchen_recommend.restype=Result
                ids=[r['id'] for r in data['recipes']]
                for row in data['draws']:
                    for name,avoid in [('result',[]),('reroll',row['result']['ids'])]:
                        previous=(C.c_uint16*len(avoid))(*[ids.index(x) for x in avoid])
                        got=dll.kitchen_recommend(recipes,len(recipes),0,row['combo'],row['minutes'],row['seed'],previous,len(avoid))
                        self.assertEqual(got.status,0,row)
                        self.assertEqual([ids[i] for i in got.indices[:got.count]],row[name]['ids'],row)
                        flags=sum(1<<i for i,n in enumerate(['主料重复','做法相同','总用时']) if any(n in text for text in row[name]['notes']))
                        self.assertEqual(got.notes,flags,row)
            finally:
                if os.name=='nt':
                    import _ctypes
                    _ctypes.FreeLibrary(dll._handle)
if __name__=='__main__':unittest.main()
