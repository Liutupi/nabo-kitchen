import ctypes as C
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
class Recipe(C.Structure):
    _fields_=[('ingredients',C.c_uint64),('main',C.c_uint64),('minutes',C.c_uint16),
              ('weight',C.c_uint16),('courses',C.c_uint8),('method',C.c_uint8),('repeated_main',C.c_uint8)]
class Result(C.Structure):
    _fields_=[('status',C.c_int),('indices',C.c_uint16*4),('count',C.c_uint8),('notes',C.c_uint8)]

def build(directory, source):
    path=Path(directory)/'recommend.c'; path.write_text(source)
    lib=Path(directory)/'recommend.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',
                    '-I',str(ROOT/'model'),str(path),str(ROOT/'model/kitchen_model.c'),
                    '-o',str(lib)],check=True)
    dll=C.CDLL(str(lib)); dll.kitchen_recommend.argtypes=[C.POINTER(Recipe),C.c_size_t,C.c_uint64,
        C.c_uint,C.c_uint,C.c_uint32,C.POINTER(C.c_uint16),C.c_size_t]
    dll.kitchen_recommend.restype=Result
    return dll

def replay(dll,cases):
    for case_no,case in enumerate(cases):
        recipes=case['recipes']; ids=[r['id'] for r in recipes]
        ingredients=sorted({i['id'] for r in recipes for i in r['ingredients']} | set(case['excluded']))
        assert len(ingredients)<=64
        bits={name:1<<n for n,name in enumerate(ingredients)}
        methods={name:n for n,name in enumerate(sorted({r['method'] for r in recipes}))}
        encoded=[]
        for r in recipes:
            all_bits=0; main_bits=0; repeated=0
            for i in r['ingredients']:
                all_bits |= bits[i['id']]
                if i.get('main'):
                    repeated |= bool(main_bits & bits[i['id']]); main_bits |= bits[i['id']]
            courses=sum({'meat':1,'veg':2,'soup':4}[c] for c in r['course'])
            encoded.append(Recipe(all_bits,main_bits,r['minutes'],r['weight'],courses,methods[r['method']],repeated))
        array=(Recipe*len(encoded))(*encoded)
        avoid=(C.c_uint16*len(case['avoid']))(*(ids.index(i) for i in case['avoid']))
        exclude=sum(bits[i] for i in set(case['excluded']))
        result=dll.kitchen_recommend(array,len(encoded),exclude,case['conditions']['combo'],
            case['conditions']['minutes'],case['seed'],avoid,len(avoid))
        expected=case['expected']
        status=0 if not expected['reason'] else 1 if expected['reason']=='没有符合忌口的菜谱' else 2
        notes=sum({'已放宽：主料重复':1,'已放宽：做法相同':2,'已放宽：总用时':4}[n]
                  for n in expected['notes'])
        got=(result.status,[ids[result.indices[i]] for i in range(result.count)],result.notes)
        wanted=(status,expected['ids'],notes)
        if got!=wanted: raise AssertionError(f'case {case_no}: {got!r} != {wanted!r}')

class RecommendParity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cases=json.loads(subprocess.check_output(['node',str(ROOT/'model/tests/recommend_trace.mjs')]))
        cls.source=(ROOT/'model/kitchen_recommend.c').read_text()
    def test_reference(self):
        with tempfile.TemporaryDirectory() as tmp: replay(build(tmp,self.source),self.cases)
    def test_recency_and_exclusion_stay_separate(self):
        original='r->ingredients & s->excluded'; self.assertEqual(self.source.count(original),1)
        with tempfile.TemporaryDirectory() as tmp, self.assertRaises(AssertionError):
            replay(build(tmp,self.source.replace(original,'r->main_ingredients & s->excluded')),self.cases)
    def test_mutations_of_combination_rules_are_detected(self):
        mutations=[
            ('s->avoid_filter && same_avoid(s)', '0 && same_avoid(s)'),
            ('s->length > 1 && earlier_permutation(s, 0, 0, 0)',
             '0 && earlier_permutation(s, 0, 0, 0)'),
            ('if (same_method)\n        notes |= KR_METHOD_RELAXED;',
             'if (0 && same_method)\n        notes |= KR_METHOD_RELAXED;'),
        ]
        for old,new in mutations:
            with self.subTest(mutation=old), tempfile.TemporaryDirectory() as tmp:
                self.assertEqual(self.source.count(old),1)
                with self.assertRaises(AssertionError): replay(build(tmp,self.source.replace(old,new)),self.cases)
    def test_invalid_inputs_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            dll=build(tmp,self.source)
            empty=(Recipe*65)()
            self.assertEqual(dll.kitchen_recommend(empty,65,0,0,30,1,None,0).status,3)
            self.assertEqual(dll.kitchen_recommend(None,0,0,4,30,1,None,0).status,3)
            self.assertEqual(dll.kitchen_recommend(None,0,0,0,30,1,None,1).status,3)

if __name__=='__main__': unittest.main()
