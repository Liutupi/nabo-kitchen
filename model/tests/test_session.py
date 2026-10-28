import ctypes as C
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
class Record(C.Structure): _fields_=[('recipe',C.c_uint16),('day',C.c_int32)]
class Cooked(C.Structure): _fields_=[('recipe',C.c_uint16),('count',C.c_uint32),('day',C.c_int32)]
class Store(C.Structure):
    _fields_=[('history',Record*60),('cooked',Cooked*64),('history_count',C.c_uint8),
              ('cooked_count',C.c_uint8),('has_streak',C.c_uint8),('streak_day',C.c_int32),('streak_days',C.c_uint32)]
class Session(C.Structure): _fields_=[('visited',C.c_uint16),('index',C.c_uint8),('completed',C.c_uint8)]
class Stats(C.Structure): _fields_=[(name,C.c_uint32) for name in ('unique','total','streak','today','level')]

def load(directory,source):
    path=Path(directory)/'session.c'; path.write_text(source); lib=Path(directory)/'session.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',
                    '-I',str(ROOT/'model'),str(path),'-o',str(lib)],check=True)
    dll=C.CDLL(str(lib)); dll.ks_init.argtypes=[C.POINTER(Store)]
    dll.ks_reset_session.argtypes=[C.POINTER(Session)]
    dll.ks_complete_step.argtypes=[C.POINTER(Store),C.POINTER(Session),C.c_uint16,C.c_uint,C.c_int32]
    dll.ks_get_stats.argtypes=[C.POINTER(Store),C.c_int32]; dll.ks_get_stats.restype=Stats
    return dll

def replay(dll,trace):
    store=Store(); dll.ks_init(C.byref(store)); sessions=(Session*3)(); current=0
    for number,event in enumerate(trace):
        op,value,day=event['op'],event['value'],event['day']; result=None
        if op=='select': current=value
        if op=='reset': dll.ks_reset_session(C.byref(sessions[current]))
        if op=='index': sessions[current].index=value
        if op=='step':
            n=dll.ks_complete_step(C.byref(store),C.byref(sessions[current]),current,3,day)
            result=['advanced','complete','already','incomplete','invalid','full'][n]
        session=sessions[current]
        state={'index':session.index,'completed':bool(session.completed),'visited':session.visited}
        history=[{'id':r.recipe,'day':r.day} for r in store.history[:store.history_count]]
        cooked={str(r.recipe):{'count':r.count,'day':r.day} for r in store.cooked[:store.cooked_count]}
        stats=dll.ks_get_stats(C.byref(store),day)
        got=(result,state,history,cooked,{name:getattr(stats,name) for name,_ in Stats._fields_})
        expected=(event['result'],event['session'],event['history'],event['cooked'],event['stats'])
        if got!=expected: raise AssertionError(f'step {number}: {got!r} != {expected!r}')

class SessionParity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.trace=json.loads(subprocess.check_output(['node',str(ROOT/'model/tests/session_trace.mjs')]))
        cls.source=(ROOT/'model/kitchen_session.c').read_text()
    def test_reference(self):
        with tempfile.TemporaryDirectory() as tmp: replay(load(tmp,self.source),self.trace)
    def test_jump_and_history_mutations_detected(self):
        changes=[('(s->visited & ((1u << step_count) - 1)) != (1u << step_count) - 1','0'),
                 ('store->history_count == KS_HISTORY_SIZE','store->history_count == KS_HISTORY_SIZE - 1')]
        for old,new in changes:
            with self.subTest(old=old), tempfile.TemporaryDirectory() as tmp:
                self.assertEqual(self.source.count(old),1)
                with self.assertRaises(AssertionError): replay(load(tmp,self.source.replace(old,new)),self.trace)

if __name__=='__main__': unittest.main()
