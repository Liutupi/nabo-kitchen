import ctypes as C
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from PIL import Image
ROOT=Path(__file__).resolve().parents[2];PKG=ROOT/'checkpoints/2026-10-28/packages'
class ViewTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cjson=os.environ.get('CJSON_DIR')
        if not cjson or not (Path(cjson)/'cJSON.c').is_file():raise unittest.SkipTest('Set CJSON_DIR to pinned ESP-IDF cJSON')
        cls.tmp=tempfile.TemporaryDirectory();lib=Path(cls.tmp.name)/'view.so'
        sources=sorted((ROOT/'model').glob('kitchen_*.c'))
        subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-I',str(ROOT/'model'),'-I',cjson,
                        *map(str,sources),str(ROOT/'model/tests/view_host.c'),str(Path(cjson)/'cJSON.c'),'-o',str(lib)],check=True)
        cls.dll=C.CDLL(str(lib));cls.dll.view_open.argtypes=[x for _ in range(6) for x in (C.POINTER(C.c_uint8),C.c_size_t)]
        cls.dll.view_page.argtypes=[C.c_uint,C.c_uint];cls.dll.view_draw.argtypes=[C.POINTER(C.c_uint16),C.c_int,C.c_int]
        cls.dll.view_step.argtypes=[C.c_uint]
        paths=[PKG/'kitchen_recipes.krc',Path(os.environ.get('KITCHEN_FONTS',str(PKG/'kitchen_fonts.bin'))),PKG/'kitchen_dish.bin',
               PKG/'kitchen_icons.bin',PKG/'kitchen_ui.bin',PKG/'kitchen_nabo.bin']
        cls.buffers=[];args=[]
        for path in paths:
            raw=path.read_bytes();buf=(C.c_uint8*len(raw)).from_buffer_copy(raw);cls.buffers.append(buf);args.extend([buf,len(buf)])
        assert cls.dll.view_open(*args)==1
    @classmethod
    def tearDownClass(cls):cls.tmp.cleanup()
    def test_pages_and_stripes(self):
        preview=Image.new('RGB',(240*4,320*4),(0,0,0))
        for page in range(14):
            with self.subTest(page=page):
                self.assertEqual(self.dll.view_page(page,0),1)
                full=(C.c_uint16*(240*320))();self.assertEqual(self.dll.view_draw(full,0,320),1,f'page {page} has missing glyph or asset')
                rows=[]
                for y in range(0,320,8):
                    strip=(C.c_uint16*(240*8))();self.assertEqual(self.dll.view_draw(strip,y,8),1,f'page {page}, y={y}')
                    rows.extend(strip)
                self.assertEqual(list(full),rows,f'page {page}')
                rgb=bytearray()
                for color in full:rgb.extend((((color>>11)&31)*255//31,((color>>5)&63)*255//63,(color&31)*255//31))
                preview.paste(Image.frombytes('RGB',(240,320),bytes(rgb)),((page%4)*240,(page//4)*320))
        out=ROOT/'private/kitchen-render';out.mkdir(parents=True,exist_ok=True);preview.save(out/'pages.png')
    def test_all_dishes_ingredients_and_steps_render(self):
        import json
        recipes=[json.loads(p.read_text()) for p in sorted((ROOT/'data/recipes/curated').glob('*.json'))]
        frame=(C.c_uint16*(240*320))()
        for index,recipe in enumerate(recipes):
            for page in (4,5,8):
                self.assertEqual(self.dll.view_page(page,index),1,recipe['id'])
                self.assertEqual(self.dll.view_draw(frame,0,320),1,f'{recipe["id"]}: page {page}')
            for step in range(len(recipe['steps'])):
                self.assertEqual(self.dll.view_step(step),1,f'{recipe["id"]}: step {step}')
                self.assertEqual(self.dll.view_draw(frame,0,320),1,f'{recipe["id"]}: step {step}')
if __name__=='__main__':unittest.main()
