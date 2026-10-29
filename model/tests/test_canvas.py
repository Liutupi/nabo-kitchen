import ctypes as C
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_kfn import Pack as FontPack,Font,opened as open_font
from test_ksp import Pack as SpritePack,Sprite,opened as open_sprite
ROOT=Path(__file__).resolve().parents[2];PKG=ROOT/'checkpoints/2026-10-28/packages'
class Canvas(C.Structure):
    _fields_=[('pixels',C.POINTER(C.c_uint16)),('width',C.c_int),('y',C.c_int),('height',C.c_int),('error',C.c_int),
              ('clip_x0',C.c_int),('clip_y0',C.c_int),('clip_x1',C.c_int),('clip_y1',C.c_int)]
class CanvasTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory();lib=Path(cls.tmp.name)/'canvas.so'
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',
            *[str(ROOT/'model'/name) for name in ('kitchen_canvas.c','kitchen_kfn.c','kitchen_ksp.c')],'-o',str(lib)],check=True)
        cls.dll=C.CDLL(str(lib));d=cls.dll
        d.kfn_open.argtypes=[C.POINTER(FontPack),C.POINTER(C.c_uint8),C.c_size_t]
        d.kfn_find_font.argtypes=[C.POINTER(FontPack),C.c_char_p,C.POINTER(Font)]
        d.ksp_open.argtypes=[C.POINTER(SpritePack),C.POINTER(C.c_uint8),C.c_size_t]
        d.ksp_find.argtypes=[C.POINTER(SpritePack),C.c_char_p,C.POINTER(Sprite)]
        d.kg_init.argtypes=[C.POINTER(Canvas),C.POINTER(C.c_uint16),C.c_int,C.c_int,C.c_int,C.c_uint16]
        d.kg_rect.argtypes=[C.POINTER(Canvas),C.c_int,C.c_int,C.c_int,C.c_int,C.c_uint16]
        d.kg_text.argtypes=[C.POINTER(Canvas),C.POINTER(FontPack),C.POINTER(Font),C.c_int,C.c_int,C.c_int,C.c_uint16,C.c_char_p]
        d.kg_sprite.argtypes=[C.POINTER(Canvas),C.POINTER(SpritePack),C.POINTER(Sprite),C.c_int,C.c_int]
        ok,cls.font,cls.font_data=open_font(d,(PKG/'kitchen_fonts.bin').read_bytes());assert ok
        ok,cls.art,cls.art_data=open_sprite(d,(PKG/'kitchen_dish.bin').read_bytes());assert ok
        cls.face=Font();assert d.kfn_find_font(C.byref(cls.font),b'han18',C.byref(cls.face))
        cls.sprite=Sprite();assert d.ksp_find(C.byref(cls.art),b'vessel_plate',C.byref(cls.sprite))
    @classmethod
    def tearDownClass(cls):cls.tmp.cleanup()
    def draw(self,y,h):
        pixels=(C.c_uint16*(240*h))();c=Canvas();d=self.dll
        d.kg_init(C.byref(c),pixels,240,y,h,0x10c6)
        d.kg_rect(C.byref(c),8,20,224,200,0xff9c)
        self.assertTrue(d.kg_sprite(C.byref(c),C.byref(self.art),C.byref(self.sprite),40,80))
        self.assertGreater(d.kg_text(C.byref(c),C.byref(self.font),C.byref(self.face),12,30,208,0x1084,
                                      '番茄炒蛋，准备好再开始。\n中火翻炒。'.encode()),0)
        self.assertEqual(c.error,0)
        return list(pixels)
    def test_stripes_equal_full_frame(self):
        full=self.draw(0,320);stripes=[]
        for y in range(0,320,8):stripes.extend(self.draw(y,8))
        self.assertEqual(stripes,full)
        self.assertGreater(len(set(full)),10)
    def test_missing_glyph_and_invalid_utf8_are_errors(self):
        pixels=(C.c_uint16*1920)();c=Canvas();self.dll.kg_init(C.byref(c),pixels,240,0,8,0)
        for text in (b'\xff', '😀'.encode()):
            self.assertEqual(self.dll.kg_text(C.byref(c),C.byref(self.font),C.byref(self.face),0,0,100,0xffff,text),-1)
            self.assertEqual(c.error,1)
if __name__=='__main__':unittest.main()
