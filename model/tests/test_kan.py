import binascii
import ctypes as C
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
from animation_codec import unpack_animations
class Pack(C.Structure):
    _fields_=[('data',C.POINTER(C.c_uint8)),('size',C.c_size_t)]+[(n,C.c_uint32) for n in ('width','height','fps','palette_count','action_count','index_offset')]
class Action(C.Structure): _fields_=[('frames',C.c_uint16),('loop',C.c_uint16),('offset',C.c_uint32),('length',C.c_uint32)]
class Player(C.Structure):
    _fields_=[('pack',C.POINTER(Pack)),('action',Action),('frame',C.POINTER(C.c_uint8)),('capacity',C.c_size_t),
              ('position',C.c_size_t),('next',C.c_uint16),('invalid',C.c_int)]
def load(tmp):
    path=Path(tmp)/'kan.so';subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',
        '-shared','-fPIC',str(ROOT/'model/kitchen_kan.c'),'-o',str(path)],check=True)
    dll=C.CDLL(str(path));dll.kan_open.argtypes=[C.POINTER(Pack),C.POINTER(C.c_uint8),C.c_size_t]
    dll.kan_find.argtypes=[C.POINTER(Pack),C.c_char_p,C.POINTER(Action)]
    dll.kan_start.argtypes=[C.POINTER(Player),C.POINTER(Pack),C.POINTER(Action),C.POINTER(C.c_uint8),C.c_size_t]
    dll.kan_next.argtypes=[C.POINTER(Player)]
    return dll
def opened(dll,data):
    buf=(C.c_uint8*len(data)).from_buffer_copy(data);p=Pack();return dll.kan_open(C.byref(p),buf,len(data)),p,buf
class AnimationReader(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory();cls.dll=load(cls.tmp.name)
        cls.data=(ROOT/'checkpoints/2026-10-28/packages/kitchen_nabo.bin').read_bytes()
    @classmethod
    def tearDownClass(cls):cls.tmp.cleanup()
    def test_all_actual_frames(self):
        reference=unpack_animations(self.data);ok,pack,keep=opened(self.dll,self.data);self.assertEqual(ok,1)
        total=0
        for name,action in reference.actions.items():
            a=Action();self.assertEqual(self.dll.kan_find(C.byref(pack),name.encode(),C.byref(a)),1)
            self.assertEqual(a.frames,len(action['frames']))
            frame=(C.c_uint8*(pack.width*pack.height))();player=Player()
            self.assertEqual(self.dll.kan_start(C.byref(player),C.byref(pack),C.byref(a),frame,len(frame)),1)
            for expected in action['frames']:
                self.assertEqual(self.dll.kan_next(C.byref(player)),1);self.assertEqual(bytes(frame),expected);total+=1
            self.assertEqual(self.dll.kan_next(C.byref(player)),0)
        self.assertEqual(total,42)
        self.assertEqual(self.dll.kan_find(C.byref(pack),b'unknown',C.byref(Action())),0)
    def test_corruption_and_memory_bounds(self):
        for n in (0,51,len(self.data)-1):self.assertEqual(opened(self.dll,self.data[:n])[0],0)
        bad=bytearray(self.data);bad[-1]^=1;self.assertEqual(opened(self.dll,bad)[0],0)
        ok,p,keep=opened(self.dll,self.data);a=Action();self.assertEqual(self.dll.kan_find(C.byref(p),b'idle',C.byref(a)),1)
        self.assertEqual(self.dll.kan_start(C.byref(Player()),C.byref(p),C.byref(a),(C.c_uint8*8)(),8),0)
        # Modify a frame CRC, then repair both stream and outer CRCs; decoding must reject it.
        bad=bytearray(self.data);io=struct.unpack_from('<I',bad,40)[0];offset,length=struct.unpack_from('<II',bad,io+20)
        bad[offset+14]^=1;struct.pack_into('<I',bad,io+28,binascii.crc32(bad[offset:offset+length]))
        struct.pack_into('<I',bad,48,binascii.crc32(bad[52:]));ok,p,keep=opened(self.dll,bad);self.assertEqual(ok,1)
        name=bytes(bad[io:io+16]).split(b'\0')[0];a=Action();self.assertEqual(self.dll.kan_find(C.byref(p),name,C.byref(a)),1)
        frame=(C.c_uint8*(p.width*p.height))();player=Player();self.assertEqual(self.dll.kan_start(C.byref(player),C.byref(p),C.byref(a),frame,len(frame)),1)
        self.assertEqual(self.dll.kan_next(C.byref(player)),-1);self.assertEqual(self.dll.kan_next(C.byref(player)),-1)
if __name__=='__main__':unittest.main()
