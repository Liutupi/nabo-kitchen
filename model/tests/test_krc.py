import binascii
import ctypes as C
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2]
PACKAGE=ROOT/'checkpoints/2026-10-28/packages/kitchen_recipes.krc'
class Pack(C.Structure):
    _fields_=[('data',C.POINTER(C.c_uint8)),('size',C.c_size_t),('count',C.c_uint32),
              ('dictionary_offset',C.c_uint32),('dictionary_length',C.c_uint32),('bitmap_offset',C.c_uint32),
              ('bitmap_width',C.c_uint32),('body_offset',C.c_uint32)]
class Index(C.Structure):
    _fields_=[('id_hash',C.c_uint32),('tags',C.c_uint16),('courses',C.c_uint8),
              ('minutes',C.c_uint8),('difficulty',C.c_uint8),('bitmap',C.POINTER(C.c_uint8))]
def library(tmp):
    lib=Path(tmp)/'krc.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',
                    str(ROOT/'model/kitchen_krc.c'),'-o',str(lib)],check=True)
    dll=C.CDLL(str(lib));dll.krc_open.argtypes=[C.POINTER(Pack),C.POINTER(C.c_uint8),C.c_size_t]
    dll.krc_index_at.argtypes=[C.POINTER(Pack),C.c_uint32,C.POINTER(Index)]
    dll.krc_read_body.argtypes=[C.POINTER(Pack),C.c_uint32,C.POINTER(C.c_uint8),C.c_size_t]
    return dll
def open_pack(dll,data):
    buf=(C.c_uint8*len(data)).from_buffer_copy(data);pack=Pack()
    ok=dll.krc_open(C.byref(pack),buf,len(data))
    return ok,pack,buf
def read_body(dll,pack,i):
    buf=(C.c_uint8*2048)();n=dll.krc_read_body(C.byref(pack),i,buf,len(buf))
    return n,bytes(buf[:max(n,0)])
def translate(r,d):
    r['id']=d['ids'][r['id']]
    for key,group in [('course','courses'),('tags','tags')]:
        if key in r:r[key]=[d[group][i] for i in r[key]]
    for key,group in [('cuisine','cuisines'),('source','sources')]:
        if key in r:r[key]=d[group][r[key]]
    for item in r['ingredients']:item['id']=d['ingredients'][item['id']]
    for item in r['seasonings']:item['id']=d['seasonings'][item['id']]
    for step in r['steps']:
        step['method']=d['methods'][step['method']];step['heat']=d['heats'][step['heat']]
    dish=r['dish'];dish['vessel']=d['vessels'][dish['vessel']]
    if dish['sauce'] is not None:dish['sauce']=d['sauces'][dish['sauce']]
    dish['layers']=[d['assets'][i] for i in dish['layers']]
    if 'garnish' in dish:dish['garnish']=[d['garnishes'][i] for i in dish['garnish']]
    return r
def reseal(data):struct.pack_into('<I',data,44,binascii.crc32(data[48:]))
class KrcReader(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.tmp=tempfile.TemporaryDirectory();cls.dll=library(cls.tmp.name);cls.data=PACKAGE.read_bytes()
    @classmethod
    def tearDownClass(cls):cls.tmp.cleanup()
    def test_all_real_recipes_match_sources(self):
        ok,p,keep=open_pack(self.dll,self.data);self.assertEqual(ok,1);self.assertEqual(p.count,30)
        dictionary=json.loads(self.data[p.dictionary_offset:p.dictionary_offset+p.dictionary_length])
        for i in range(p.count):
            info=Index();self.assertEqual(self.dll.krc_index_at(C.byref(p),i,C.byref(info)),1)
            n,raw=read_body(self.dll,p,i);self.assertGreater(n,0)
            result=translate(json.loads(raw),dictionary)
            source=json.loads((ROOT/'data/recipes/curated'/f'{result["id"]}.json').read_text())
            self.assertEqual(result,source);self.assertEqual(info.minutes,source['minutes'])
            self.assertEqual(info.difficulty,source['difficulty'])
            bitmap=bytes(info.bitmap[:p.bitmap_width])
            for item in source['ingredients']:
                ordinal=dictionary['ingredients'].index(item['id'])
                if item.get('main'):self.assertTrue(bitmap[ordinal//8]&(1<<(ordinal%8)))
        self.assertEqual(self.dll.krc_index_at(C.byref(p),p.count,C.byref(Index())),0)
        self.assertEqual(self.dll.krc_read_body(C.byref(p),0,(C.c_uint8*1)(),1),-1)
    def test_corrupt_sections_and_bodies_fail(self):
        for cut in (0,1,47,48,len(self.data)-1):
            self.assertEqual(open_pack(self.dll,self.data[:cut])[0],0)
        bad=bytearray(self.data);bad[-1]^=1;self.assertEqual(open_pack(self.dll,bad)[0],0)
        bad=bytearray(self.data);bad[48+9]=0xff;reseal(bad);self.assertEqual(open_pack(self.dll,bad)[0],0)
        bad=bytearray(self.data);struct.pack_into('<I',bad,36,0xffffffff);self.assertEqual(open_pack(self.dll,bad)[0],0)
        # Resealed outer CRC must not hide a bad per-body checksum.
        bad=bytearray(self.data);body=struct.unpack_from('<I',bad,36)[0];bad[body+4]^=1;reseal(bad)
        ok,p,keep=open_pack(self.dll,bad);self.assertEqual(ok,1);self.assertEqual(read_body(self.dll,p,0)[0],-1)
        # Resealed payload with a decoded-size overrun is rejected on open.
        bad=bytearray(self.data);struct.pack_into('<H',bad,body+2,2049);reseal(bad)
        self.assertEqual(open_pack(self.dll,bad)[0],0)
if __name__=='__main__':unittest.main()
