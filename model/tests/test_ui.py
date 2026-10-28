import ctypes as C
from datetime import date
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2]
class Shopping(C.Structure):
    _fields_=[('uid',C.c_uint32),('qty100',C.c_uint32),('id',C.c_uint8),('unit',C.c_uint8),('bought',C.c_uint8)]
READ=C.CFUNCTYPE(C.c_int,C.c_void_p,C.c_uint,C.POINTER(C.c_uint8),C.c_size_t,C.POINTER(C.c_size_t))
WRITE=C.CFUNCTYPE(C.c_int,C.c_void_p,C.c_uint,C.POINTER(C.c_uint8),C.c_size_t)
class JournalIO(C.Structure):_fields_=[('context',C.c_void_p),('read',READ),('write',WRITE)]
class UiParity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cjson=os.environ.get('CJSON_DIR')
        if not cjson or not (Path(cjson)/'cJSON.c').is_file():raise unittest.SkipTest('Set CJSON_DIR to pinned ESP-IDF cJSON')
        cls.trace=json.loads(subprocess.check_output(['node',str(ROOT/'model/tests/ui_trace.cjs')]))
        cls.tmp=tempfile.TemporaryDirectory();library=Path(cls.tmp.name)/'ui.so'
        sources=['kitchen_ui.c','kitchen_lists.c','kitchen_model.c','kitchen_recommend.c','kitchen_session.c','kitchen_timer.c','kitchen_krc.c','kitchen_catalog.c','kitchen_metadata.c','kitchen_save.c','kitchen_journal.c']
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC','-I',str(ROOT/'model'),'-I',cjson,
                        *[str(ROOT/'model'/x) for x in sources],str(ROOT/'model/tests/ui_host.c'),str(Path(cjson)/'cJSON.c'),'-o',str(library)],check=True)
        cls.dll=C.CDLL(str(library));cls.dll.host_init.argtypes=[C.POINTER(C.c_uint8),C.c_size_t,C.c_int32,C.c_uint,C.c_uint32,
            C.POINTER(C.c_uint8),C.c_uint,C.POINTER(C.c_uint8),C.c_uint]
        cls.dll.host_key.argtypes=[C.c_int,C.c_int,C.c_int64];cls.dll.host_tick.argtypes=[C.c_int64]
        cls.dll.host_snapshot.restype=C.c_char_p
        cls.dll.host_set_fridge.argtypes=[C.c_uint64,C.c_uint64]
        cls.dll.host_fridge.argtypes=[C.POINTER(C.c_uint16),C.c_uint];cls.dll.host_fridge.restype=C.c_uint
        cls.dll.host_add_shopping.argtypes=[C.c_uint,C.c_uint];cls.dll.host_bought.argtypes=[C.c_uint]
        cls.dll.host_shopping.argtypes=[C.POINTER(Shopping),C.c_uint];cls.dll.host_shopping.restype=C.c_uint
        cls.dll.host_save.argtypes=[C.POINTER(C.c_uint8),C.c_size_t,C.c_uint32,C.c_int64,C.c_int64];cls.dll.host_save.restype=C.c_size_t
        cls.dll.host_load.argtypes=[C.POINTER(C.c_uint8),C.c_size_t,C.POINTER(C.c_uint32),C.c_int64,C.c_int64,C.POINTER(C.c_int)]
        cls.dll.host_save_info.argtypes=[C.c_int64];cls.dll.host_save_info.restype=C.c_char_p
        cls.dll.host_journal_load.argtypes=[C.POINTER(JournalIO),C.c_int64,C.c_int64,C.POINTER(C.c_int)]
        cls.dll.host_journal_store.argtypes=[C.POINTER(JournalIO),C.c_int64,C.c_int64]
        blob=(ROOT/'checkpoints/2026-10-28/packages/kitchen_recipes.krc').read_bytes()
        cls.buf=(C.c_uint8*len(blob)).from_buffer_copy(blob)
        import sys
        sys.path.insert(0,str(ROOT/'tools'));from build_firmware_metadata import build
        meta=json.loads(build());cls.order=(C.c_uint8*len(meta['ingredient_order']))(*meta['ingredient_order'])
        cls.fridge=(C.c_uint8*len(meta['fridge_order']))(*meta['fridge_order'])
        cls.lists=json.loads(subprocess.check_output(['node',str(ROOT/'model/tests/lists_trace.mjs')]))
    @classmethod
    def tearDownClass(cls):cls.tmp.cleanup()
    def test_actual_browser_flow(self):
        seed=2166136261
        for char in '2026-10-28|browser-01':seed=((seed^ord(char))*16777619)&0xffffffff
        days=(date(2026,10,28)-date(1970,1,1)).days
        self.assertEqual(self.dll.host_init(self.buf,len(self.buf),days,10,seed,self.order,len(self.order),self.fridge,len(self.fridge)),1)
        for index,e in enumerate(self.trace):
            if e['op']!='init':
                now=e['now']-(1600 if e['op']=='key_transition' else 0)
                self.dll.host_key(['UP','DOWN','OK'].index(e['key']),['short','double','long'].index(e['kind']),now)
                if e['op']=='key_transition':self.dll.host_tick(e['now'])
            actual=json.loads(self.dll.host_snapshot());actual.pop('message')
            self.assertEqual(actual,e['expected'],f'trace {index}: {e}')
    def test_fridge_and_shopping_against_prototype(self):
        days=(date(2026,10,28)-date(1970,1,1)).days
        self.assertEqual(self.dll.host_init(self.buf,len(self.buf),days,10,1,self.order,len(self.order),self.fridge,len(self.fridge)),1)
        for case in self.lists['fridge']:
            self.dll.host_set_fridge(int(case['have']),int(case['excluded']))
            out=(C.c_uint16*64)();count=self.dll.host_fridge(out,len(out))
            self.assertEqual(list(out[:count]),case['ids'])
        for case in self.lists['shopping']:
            if case['op']=='add':self.assertEqual(self.dll.host_add_shopping(case['recipe'],case['people']),1)
            else:self.dll.host_bought(0)
            out=(Shopping*100)();count=self.dll.host_shopping(out,len(out))
            actual=[{name:getattr(row,name) for name,_ in Shopping._fields_} for row in out[:count]]
            self.assertEqual(actual,case['items'])
    def test_save_restore_and_clock_loss(self):
        days=(date(2026,10,28)-date(1970,1,1)).days
        self.assertEqual(self.dll.host_init(self.buf,len(self.buf),days,10,1,self.order,len(self.order),self.fridge,len(self.fridge)),1)
        self.dll.host_prepare_save();original=json.loads(self.dll.host_save_info(2500));saved=(C.c_uint8*8192)()
        size=self.dll.host_save(saved,len(saved),77,2500,1800000000000);self.assertGreater(size,20)
        self.dll.host_reverse_catalog();seq=C.c_uint32();lost=C.c_int()
        self.assertEqual(self.dll.host_load(saved,size,C.byref(seq),10000,1800000005000,C.byref(lost)),1)
        restored=json.loads(self.dll.host_save_info(10000));self.assertEqual(seq.value,77);self.assertEqual(lost.value,0)
        for key in ('people','minutes','combo','saved0','saved1','favorites','history','shopping','timers','status0','status1','left1'):
            self.assertEqual(restored[key],original[key],key)
        self.assertEqual(restored['left0'],original['left0']-5000)
        self.assertEqual(self.dll.host_load(saved,size,C.byref(seq),10000,-1,C.byref(lost)),1)
        paused=json.loads(self.dll.host_save_info(10000));self.assertEqual(lost.value,1)
        self.assertEqual(paused['status0'],1);self.assertEqual(paused['left0'],original['left0'])
        before=self.dll.host_save_info(10000);bad=(C.c_uint8*size).from_buffer_copy(bytes(saved[:size]));bad[size-1]^=1
        self.assertEqual(self.dll.host_load(bad,size,C.byref(seq),10000,1800000005000,C.byref(lost)),0)
        self.assertEqual(self.dll.host_save_info(10000),before)
    def test_journal_interruption_and_incompatible_data(self):
        days=(date(2026,10,28)-date(1970,1,1)).days
        self.assertEqual(self.dll.host_init(self.buf,len(self.buf),days,10,1,self.order,len(self.order),self.fridge,len(self.fridge)),1)
        slots={};fail=[False];read_error=[False]
        @READ
        def read(_,slot,out,cap,length):
            if read_error[0]:return -1
            if slot not in slots:return 0
            data=slots[slot]
            if len(data)>cap:return -1
            C.memmove(out,data,len(data));length[0]=len(data);return 1
        @WRITE
        def write(_,slot,data,length):
            raw=bytes(data[:length])
            if fail[0]:slots[slot]=raw[:len(raw)//2];return 0
            slots[slot]=raw;return 1
        io=JournalIO(None,read,write);lost=C.c_int()
        self.assertEqual(self.dll.host_journal_load(C.byref(io),2500,1800000000000,C.byref(lost)),1)
        self.dll.host_prepare_save();self.assertEqual(self.dll.host_journal_store(C.byref(io),2500,1800000000000),0)
        self.dll.host_bought(0);self.assertEqual(self.dll.host_journal_store(C.byref(io),2500,1800000000000),0)
        fail[0]=True;self.assertEqual(self.dll.host_journal_store(C.byref(io),2500,1800000000000),2);fail[0]=False
        self.assertEqual(self.dll.host_journal_load(C.byref(io),2500,1800000000000,C.byref(lost)),6)
        info=json.loads(self.dll.host_save_info(2500));self.assertGreater(info['shopping'],0)
        out=(Shopping*100)();self.dll.host_shopping(out,len(out));self.assertEqual(out[0].bought,1)
        # A valid newest record from an unknown version blocks overwrite.
        latest=bytearray(slots[1]);latest[4]=2;slots[1]=bytes(latest)
        self.assertEqual(self.dll.host_journal_load(C.byref(io),2500,1800000000000,C.byref(lost)),4)
        self.assertEqual(self.dll.host_journal_store(C.byref(io),2500,1800000000000),2)
        read_error[0]=True;self.assertEqual(self.dll.host_journal_load(C.byref(io),2500,1800000000000,C.byref(lost)),2)
if __name__=='__main__':unittest.main()
