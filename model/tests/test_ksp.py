import binascii
import ctypes as C
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
from asset_codec import unpack_sprites
class Pack(C.Structure):
    _fields_=[('data',C.POINTER(C.c_uint8)),('size',C.c_size_t),('groups',C.c_uint32),('count',C.c_uint32),
              ('index_offset',C.c_uint32),('strings_offset',C.c_uint32),('payload_offset',C.c_uint32)]
class Sprite(C.Structure):
    _fields_=[(x,C.c_uint16) for x in ('frame_width','frame_height','x','y','width','height','palette_count','codec')]+[
        ('palette',C.POINTER(C.c_uint8)),('offset',C.c_uint32),('length',C.c_uint32),('crc',C.c_uint32)]
CALL=C.CFUNCTYPE(C.c_int,C.c_void_p,C.c_uint,C.c_uint,C.POINTER(C.c_uint8),C.c_uint)
def load(tmp):
    path=Path(tmp)/'ksp.so';subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',
        '-shared','-fPIC',str(ROOT/'model/kitchen_ksp.c'),'-o',str(path)],check=True)
    dll=C.CDLL(str(path));dll.ksp_open.argtypes=[C.POINTER(Pack),C.POINTER(C.c_uint8),C.c_size_t]
    dll.ksp_find.argtypes=[C.POINTER(Pack),C.c_char_p,C.POINTER(Sprite)]
    dll.ksp_read_rows.argtypes=[C.POINTER(Pack),C.POINTER(Sprite),C.POINTER(C.c_uint8),C.c_size_t,CALL,C.c_void_p]
    return dll
def opened(dll,data):
    buf=(C.c_uint8*len(data)).from_buffer_copy(data);p=Pack();return dll.ksp_open(C.byref(p),buf,len(data)),p,buf
class SpriteReader(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.tmp=tempfile.TemporaryDirectory();cls.dll=load(cls.tmp.name)
    @classmethod
    def tearDownClass(cls):cls.tmp.cleanup()
    def test_all_runtime_sprites_match_reference(self):
        for filename in ('kitchen_dish.bin','kitchen_icons.bin','kitchen_ui.bin'):
            data=(ROOT/'checkpoints/2026-10-28/packages'/filename).read_bytes()
            reference=unpack_sprites(data);assets,groups=reference.assets,reference.groups
            ok,p,keep=opened(self.dll,data);self.assertEqual(ok,1)
            for name,expected in assets.items():
                sprite=Sprite();self.assertEqual(self.dll.ksp_find(C.byref(p),name.encode(),C.byref(sprite)),1)
                fw,fh=expected['frame'];frame=bytearray(fw*fh)
                @CALL
                def row(_,x,y,indices,width):frame[y*fw+x:y*fw+x+width]=bytes(indices[:width]);return 1
                line=(C.c_uint8*1024)()
                self.assertEqual(self.dll.ksp_read_rows(C.byref(p),C.byref(sprite),line,len(line),row,None),1,name)
                self.assertEqual(bytes(frame),expected['indices'],name)
                actual=[sprite.palette[i*2]|sprite.palette[i*2+1]<<8 for i in range(sprite.palette_count)]
                self.assertEqual(actual,groups[expected['group']],name)
            self.assertEqual(self.dll.ksp_find(C.byref(p),b'not-a-sprite',C.byref(Sprite())),0)
    def test_corruption(self):
        data=(ROOT/'checkpoints/2026-10-28/packages/kitchen_dish.bin').read_bytes()
        for n in [0,39,len(data)-1]:self.assertEqual(opened(self.dll,data[:n])[0],0)
        bad=bytearray(data);bad[-1]^=1;self.assertEqual(opened(self.dll,bad)[0],0)
        bad=bytearray(data);io=struct.unpack_from('<I',bad,24)[0];struct.pack_into('<H',bad,io+8,65535)
        struct.pack_into('<I',bad,36,binascii.crc32(bad[40:]));self.assertEqual(opened(self.dll,bad)[0],0)
        # A resealed pixel checksum is still verified during row decoding.
        bad=bytearray(data);bad[io+32]^=1;struct.pack_into('<I',bad,36,binascii.crc32(bad[40:]))
        ok,p,keep=opened(self.dll,bad);self.assertEqual(ok,1)
        name=next(iter(unpack_sprites(data).assets));sprite=Sprite();self.assertEqual(self.dll.ksp_find(C.byref(p),name.encode(),C.byref(sprite)),1)
        @CALL
        def row(*_):return 1
        self.assertEqual(self.dll.ksp_read_rows(C.byref(p),C.byref(sprite),(C.c_uint8*1024)(),1024,row,None),0)
if __name__=='__main__':unittest.main()
