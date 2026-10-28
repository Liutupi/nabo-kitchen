import ctypes as C
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2]
class Input(C.Structure):
    _fields_=[('pressed_at',C.c_int64*3),('pending_at',C.c_int64),('last_now',C.c_int64),
              ('sequence',C.c_uint32*3),('pending_sequence',C.c_uint32),('next_sequence',C.c_uint32),
              ('held',C.c_uint8*3),('long_sent',C.c_uint8*3),('pending',C.c_uint8),('initialized',C.c_uint8)]
CALL=C.CFUNCTYPE(None,C.c_void_p,C.c_int,C.c_int)
def load(directory,source):
    path=Path(directory)/'input.c';path.write_text(source);lib=Path(directory)/'input.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',
                    '-I',str(ROOT/'model'),str(path),'-o',str(lib)],check=True)
    dll=C.CDLL(str(lib));ptr=C.POINTER(Input);dll.ki_init.argtypes=[ptr]
    dll.ki_poll.argtypes=[ptr,C.c_int64,CALL,C.c_void_p]
    dll.ki_press.argtypes=[ptr,C.c_int,C.c_int64,CALL,C.c_void_p]
    dll.ki_release.argtypes=[ptr,C.c_int,C.c_int,C.c_int64,CALL,C.c_void_p]
    dll.ki_blur.argtypes=[ptr,C.c_int64,CALL,C.c_void_p]
    return dll
def replay(dll,trace):
    state=Input();dll.ki_init(C.byref(state));events=[]
    @CALL
    def callback(_,key,kind): events.append({'key':['UP','DOWN','OK'][key],'kind':['short','double','long'][kind]})
    for number,e in enumerate(trace):
        events.clear();p=C.byref(state);t=e['now'];key=['UP','DOWN','OK'].index(e['key'])
        if e['op']=='press': result=dll.ki_press(p,key,t,callback,None)
        elif e['op'] in ('release','cancel'): result=dll.ki_release(p,key,e['op']=='cancel',t,callback,None)
        elif e['op']=='blur': result=dll.ki_blur(p,t,callback,None)
        else: result=dll.ki_poll(p,t,callback,None)
        if result!=1 or events!=e['events']:
            raise AssertionError(f'{number} {e}: actual {events!r}, result {result}')
class InputParity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.trace=json.loads(subprocess.check_output(['node',str(ROOT/'model/tests/input_trace.mjs')]))
        cls.source=(ROOT/'model/kitchen_input.c').read_text()
    def test_reference(self):
        with tempfile.TemporaryDirectory() as tmp: replay(load(tmp,self.source),self.trace)
    def test_double_and_long_mutations_detected(self):
        for old,new in [('#define LONG_MS 650','#define LONG_MS 600'),('#define DOUBLE_MS 280','#define DOUBLE_MS 200')]:
            with self.subTest(old=old), tempfile.TemporaryDirectory() as tmp:
                self.assertEqual(self.source.count(old),1)
                with self.assertRaises(AssertionError): replay(load(tmp,self.source.replace(old,new)),self.trace)
if __name__=='__main__':unittest.main()
