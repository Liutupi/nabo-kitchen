import binascii
import ctypes as C
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
from font_codec import unpack_fonts
class Pack(C.Structure):
    _fields_=[('data',C.POINTER(C.c_uint8)),('size',C.c_size_t),('font_count',C.c_uint32),
              ('glyph_count',C.c_uint32),('glyph_offset',C.c_uint32),('payload_offset',C.c_uint32)]
class Font(C.Structure):
    _fields_=[('start',C.c_uint32),('count',C.c_uint32),('px',C.c_uint16),('bpp',C.c_uint16),
              ('ascent',C.c_int16),('descent',C.c_int16),('line_height',C.c_uint16)]
class Glyph(C.Structure):
    _fields_=[('width',C.c_uint8),('height',C.c_uint8),('advance',C.c_uint8),('x',C.c_int8),
              ('y',C.c_int8),('length',C.c_size_t)]
def load(tmp):
    path=Path(tmp)/'kfn.so';subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',
        '-shared','-fPIC',str(ROOT/'model/kitchen_kfn.c'),'-o',str(path)],check=True)
    dll=C.CDLL(str(path));dll.kfn_open.argtypes=[C.POINTER(Pack),C.POINTER(C.c_uint8),C.c_size_t]
    dll.kfn_find_font.argtypes=[C.POINTER(Pack),C.c_char_p,C.POINTER(Font)]
    dll.kfn_read_glyph.argtypes=[C.POINTER(Pack),C.POINTER(Font),C.c_uint32,C.POINTER(Glyph),C.POINTER(C.c_uint8),C.c_size_t]
    return dll
def opened(dll,data):
    buf=(C.c_uint8*len(data)).from_buffer_copy(data);p=Pack();return dll.kfn_open(C.byref(p),buf,len(data)),p,buf
class FontReader(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory();cls.dll=load(cls.tmp.name)
        cls.data=(ROOT/'checkpoints/2026-10-28/packages/kitchen_fonts.bin').read_bytes()
    @classmethod
    def tearDownClass(cls):cls.tmp.cleanup()
    def sources(self):
        yield 'frozen',self.data
        if os.environ.get('KITCHEN_FONTS'):
            yield 'candidate',Path(os.environ['KITCHEN_FONTS']).read_bytes()
    def test_all_frozen_glyphs_match_reference(self):
        for label,data in self.sources():
            with self.subTest(package=label):
                ok,p,keep=opened(self.dll,data);self.assertEqual(ok,1);reference=unpack_fonts(data)
                count=sum(len(font['glyphs']) for font in reference.fonts.values())
                self.assertEqual(p.glyph_count,count)
                if label=='frozen':self.assertEqual(p.glyph_count,12447)
                for name,font in reference.fonts.items():
                    f=Font();self.assertEqual(self.dll.kfn_find_font(C.byref(p),name.encode(),C.byref(f)),1)
                    self.assertEqual((f.px,f.bpp,f.count),(font['px'],font['bpp'],len(font['glyphs'])))
                    for cp,expected in font['glyphs'].items():
                        g=Glyph();output=(C.c_uint8*1024)()
                        self.assertEqual(self.dll.kfn_read_glyph(C.byref(p),C.byref(f),cp,C.byref(g),output,len(output)),1)
                        self.assertEqual((g.width,g.height,g.x,g.y,g.advance,bytes(output[:g.length])),
                            (expected.width,expected.height,expected.x,expected.y,expected.advance,expected.coverage))
                    self.assertEqual(self.dll.kfn_read_glyph(C.byref(p),C.byref(f),0x10ffff,C.byref(Glyph()),(C.c_uint8*1024)(),1024),0)
    def test_corruption_and_bounds(self):
        for label,data in self.sources():
            with self.subTest(package=label):
                for n in [0,1,35,len(data)-1]:self.assertEqual(opened(self.dll,data[:n])[0],0)
                bad=bytearray(data);bad[-1]^=1;self.assertEqual(opened(self.dll,bad)[0],0)
                bad=bytearray(data);struct.pack_into('<I',bad,24,0xffffffff);self.assertEqual(opened(self.dll,bad)[0],0)
                bad=bytearray(data);go=struct.unpack_from('<I',bad,20)[0];bad[go+9]=2
                struct.pack_into('<I',bad,32,binascii.crc32(bad[36:]));self.assertEqual(opened(self.dll,bad)[0],0)
                ok,p,keep=opened(self.dll,data);f=Font();self.assertEqual(self.dll.kfn_find_font(C.byref(p),b'not-a-font',C.byref(f)),0)
if __name__=='__main__':unittest.main()
