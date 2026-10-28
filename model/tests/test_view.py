import ctypes as C
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from package_fixtures import FROZEN,FROZEN_META,selected,recipe_ids
from PIL import Image
ROOT=Path(__file__).resolve().parents[2];PKG=ROOT/'checkpoints/2026-10-28/packages'
class ViewTest(unittest.TestCase):
    recipe_path=FROZEN
    metadata_path=FROZEN_META
    complete=False
    @classmethod
    def setUpClass(cls):
        cjson=os.environ.get('CJSON_DIR')
        if not cjson or not (Path(cjson)/'cJSON.c').is_file():raise unittest.SkipTest('Set CJSON_DIR to pinned ESP-IDF cJSON')
        cls.tmp=tempfile.TemporaryDirectory();lib=Path(cls.tmp.name)/'view.so'
        sources=[x for x in sorted((ROOT/'model').glob('kitchen_*.c')) if x.name!='kitchen_metadata.c']+[cls.metadata_path]
        cls.ids=recipe_ids(cls.recipe_path,cls.complete)
        if not cls.complete:assert len(cls.ids)==30
        subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-I',str(ROOT/'model'),'-I',cjson,
                        *map(str,sources),str(ROOT/'model/tests/view_host.c'),str(Path(cjson)/'cJSON.c'),'-o',str(lib)],check=True)
        cls.dll=C.CDLL(str(lib));cls.dll.view_open.argtypes=[x for _ in range(6) for x in (C.POINTER(C.c_uint8),C.c_size_t)]
        cls.dll.view_page.argtypes=[C.c_uint,C.c_uint];cls.dll.view_draw.argtypes=[C.POINTER(C.c_uint16),C.c_int,C.c_int]
        cls.dll.view_step.argtypes=[C.c_uint]
        cls.dll.view_combo.argtypes=[C.c_uint];cls.dll.view_paper_scroll.argtypes=[C.c_uint]
        cls.dll.view_step_timer.argtypes=[C.c_uint];cls.dll.view_scene_time.argtypes=[C.c_uint]
        cls.dll.view_boot_at.argtypes=[C.c_uint];cls.dll.view_boot_page.argtypes=[C.c_uint]
        icon_path=os.environ.get('KITCHEN_ICONS')
        if not icon_path:
            # The checkpoint is intentionally frozen; rebuild the current icon
            # pool into the temporary test folder when testing source changes.
            sys.path.insert(0,str(ROOT/'tools'))
            from check_recipes import asset_rows
            from pack_assets import profile
            from asset_codec import prepare_image, pack_sprites
            entries=[]
            for row in asset_rows(ROOT).values():
                if not row['path'].endswith('.png'):continue
                pool,group,size,character,key=profile(row)
                if pool=='icons':
                    entries.append({'id':row['id'],'group':group,'prepared':prepare_image(
                        ROOT/'game-art/kitchen'/row['path'],size,character,key)})
            icon_path=str(Path(cls.tmp.name)/'kitchen_icons.bin')
            Path(icon_path).write_bytes(pack_sprites(entries))
        paths=[cls.recipe_path,Path(os.environ.get('KITCHEN_FONTS',str(PKG/'kitchen_fonts.bin'))),PKG/'kitchen_dish.bin',
               Path(icon_path),PKG/'kitchen_ui.bin',Path(os.environ.get('KITCHEN_ANIMATION',str(PKG/'kitchen_nabo.bin')))]
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
    def test_boot_timeline_and_stripes(self):
        import hashlib
        self.assertEqual(self.dll.view_page(0,0),1)
        self.assertEqual(self.dll.view_boot_page(0),0)
        self.assertEqual(self.dll.view_boot_page(3599),0)
        self.assertEqual(self.dll.view_boot_page(3600),1)
        self.assertEqual(self.dll.view_boot_skip(),1)
        preview=Image.new('RGB',(240*4,320),(0,0,0))
        hashes=[]
        for i,elapsed in enumerate((0,900,1700,2900)):
            self.assertEqual(self.dll.view_boot_at(elapsed),1)
            full=(C.c_uint16*(240*320))()
            self.assertEqual(self.dll.view_draw(full,0,320),1,f'boot {elapsed} has missing glyph or asset')
            rows=[]
            for y in range(0,320,8):
                strip=(C.c_uint16*(240*8))()
                self.assertEqual(self.dll.view_draw(strip,y,8),1)
                rows.extend(strip)
            self.assertEqual(list(full),rows,f'boot {elapsed} stripes')
            hashes.append(hashlib.sha256(bytes(full)).digest())
            rgb=bytearray()
            for color in full:rgb.extend((((color>>11)&31)*255//31,((color>>5)&63)*255//63,(color&31)*255//31))
            preview.paste(Image.frombytes('RGB',(240,320),bytes(rgb)),(i*240,0))
        self.assertEqual(len(set(hashes)),4,'each stage must change the drawn frame')
        out=ROOT/'private/kitchen-render';out.mkdir(parents=True,exist_ok=True);preview.save(out/'boot-timeline.png')
    def test_detail_states_and_stripes(self):
        import hashlib, json
        recipes=[json.loads((ROOT/'data/recipes/curated'/f'{i}.json').read_text()) for i in self.ids]
        def checked():
            full=(C.c_uint16*(240*320))()
            self.assertEqual(self.dll.view_draw(full,0,320),1)
            pieces=[]
            for y in range(0,320,8):
                part=(C.c_uint16*(240*8))();self.assertEqual(self.dll.view_draw(part,y,8),1)
                pieces.extend(part)
            self.assertEqual(list(full),pieces)
            return hashlib.sha256(bytes(full)).digest()
        self.assertEqual(self.dll.view_page(4,0),1)
        groups=[]
        for selected in range(4):
            self.assertEqual(self.dll.view_combo(selected),1)
            groups.append(checked())
        self.assertEqual(len(set(groups)),4,'each highlighted dish must be distinct')
        longest=max(range(len(recipes)),key=lambda i:len(recipes[i]['ingredients'])*36+len(recipes[i].get('seasonings',[]))*24)
        self.assertEqual(self.dll.view_page(5,longest),1)
        top=checked();last=self.dll.view_paper_max();self.assertGreater(last,0)
        self.assertEqual(self.dll.view_paper_scroll(last),1)
        self.assertNotEqual(top,checked(),'scroll must expose different ingredients and seasoning')
        self.assertEqual(self.dll.view_page(6,0),1);self.assertEqual(self.dll.view_step(2),1)
        states=[]
        for kind in range(3):
            self.assertEqual(self.dll.view_step_timer(kind),1)
            states.append(checked())
        self.assertEqual(len(set(states)),3,'running, paused and done timers must be distinguishable')
        self.assertEqual(self.dll.view_page(8,0),1);empty=checked()
        self.assertEqual(self.dll.view_complete_progress(),1)
        complete=checked()
        self.assertNotEqual(empty,complete,'completion history must update its visual progress')
        self.assertEqual(self.dll.view_scene_time(300),1)
        self.assertNotEqual(complete,checked(),'completion confetti must move with time')
    def test_all_dishes_ingredients_and_steps_render(self):
        import json
        recipes=[json.loads((ROOT/'data/recipes/curated'/f'{i}.json').read_text()) for i in self.ids]
        frame=(C.c_uint16*(240*320))()
        for index,recipe in enumerate(recipes):
            for page in (4,5,8):
                self.assertEqual(self.dll.view_page(page,index),1,recipe['id'])
                self.assertEqual(self.dll.view_draw(frame,0,320),1,f'{recipe["id"]}: page {page}')
                if page==5:
                    maximum=self.dll.view_paper_max()
                    if maximum:
                        self.assertEqual(self.dll.view_paper_scroll(maximum),1,recipe['id'])
                        self.assertEqual(self.dll.view_draw(frame,0,320),1,f'{recipe["id"]}: scrolled ingredient list')
            for step in range(len(recipe['steps'])):
                self.assertEqual(self.dll.view_step(step),1,f'{recipe["id"]}: step {step}')
                self.assertEqual(self.dll.view_draw(frame,0,320),1,f'{recipe["id"]}: step {step}')
if selected():
    class ViewTestSelected(ViewTest):
        recipe_path,metadata_path=selected()
        complete=True
if __name__=='__main__':unittest.main()
