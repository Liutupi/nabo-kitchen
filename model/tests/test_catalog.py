import ctypes as C
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_krc import Pack,open_pack
from package_fixtures import FROZEN,FROZEN_META,selected,recipe_ids
from test_recommend import Recipe
ROOT=Path(__file__).resolve().parents[2]
class Item(C.Structure):
    _fields_=[('recipe',Recipe),('hash',C.c_uint32),('id',C.c_char*41),('name',C.c_char*41),
              ('season',C.c_uint16),('tags',C.c_uint16),('servings',C.c_uint8),('difficulty',C.c_uint8)]
class Catalog(C.Structure):_fields_=[('items',C.POINTER(Item)),('capacity',C.c_uint),('count',C.c_uint)]
class Step(C.Structure):_fields_=[('text',C.c_char*193),('tip',C.c_char*65),('timer',C.c_uint32),('method',C.c_uint8),('heat',C.c_uint8)]
class Ingredient(C.Structure):_fields_=[('qty100',C.c_uint32),('id',C.c_uint8),('unit',C.c_uint8),('main',C.c_uint8)]
class Dish(C.Structure):_fields_=[('vessel',C.c_uint8),('layer_count',C.c_uint8),('garnish_count',C.c_uint8),
    ('sauce',C.c_int16),('layers',C.c_uint16*4),('garnishes',C.c_uint16*3)]
class CatalogReader(unittest.TestCase):
    recipe_path=FROZEN
    metadata_path=FROZEN_META
    complete=False
    @classmethod
    def setUpClass(cls):
        cjson=os.environ.get('CJSON_DIR')
        if not cjson or not (Path(cjson)/'cJSON.c').is_file():raise unittest.SkipTest('Set CJSON_DIR to the pinned ESP-IDF cJSON source directory')
        cls.tmp=tempfile.TemporaryDirectory();lib=Path(cls.tmp.name)/'catalog.so'
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC','-I',cjson,'-I',str(ROOT/'model'),
            str(ROOT/'model/kitchen_krc.c'),str(ROOT/'model/kitchen_catalog.c'),str(cls.metadata_path),str(Path(cjson)/'cJSON.c'),'-o',str(lib)],check=True)
        cls.dll=C.CDLL(str(lib));cls.dll.krc_open.argtypes=[C.POINTER(Pack),C.POINTER(C.c_uint8),C.c_size_t]
        cls.dll.kc_load.argtypes=[C.POINTER(Catalog),C.POINTER(Pack)]
        cls.dll.kc_read_step.argtypes=[C.POINTER(Pack),C.c_uint,C.c_uint,C.POINTER(Step),C.POINTER(C.c_uint)]
        cls.dll.kc_read_ingredients.argtypes=[C.POINTER(Pack),C.c_uint,C.POINTER(Ingredient),C.c_uint,C.POINTER(C.c_uint)]
        cls.dll.kc_read_dish.argtypes=[C.POINTER(Pack),C.c_uint,C.POINTER(Dish)]
        cls.data=cls.recipe_path.read_bytes()
        cls.ids=recipe_ids(cls.recipe_path,cls.complete)
        if not cls.complete:assert len(cls.ids)==30
        by_id={x['id']:x for x in json.loads(subprocess.check_output(['node',str(ROOT/'model/tests/catalog_trace.mjs')]))}
        cls.expected=[by_id[i] for i in cls.ids]
    @classmethod
    def tearDownClass(cls):cls.tmp.cleanup()
    def test_actual_catalog_matches_prototype(self):
        ok,pack,keep=open_pack(self.dll,self.data);self.assertEqual(ok,1)
        items=(Item*pack.count)();catalog=Catalog(items,len(items),0)
        self.assertEqual(self.dll.kc_load(C.byref(catalog),C.byref(pack)),1);self.assertEqual(catalog.count,len(self.ids))
        dictionary=json.loads(self.data[pack.dictionary_offset:pack.dictionary_offset+pack.dictionary_length])
        for i,expected in enumerate(self.expected):
            item=catalog.items[i];self.assertEqual(item.id.decode(),expected['id']);self.assertEqual(item.name.decode(),expected['name'])
            self.assertEqual(dictionary['methods'][item.recipe.method],expected['primary'])
            source=json.loads((ROOT/'data/recipes/curated'/f'{expected["id"]}.json').read_text())
            bit=lambda x:1<<dictionary['ingredients'].index(x['id'])
            self.assertEqual(item.recipe.ingredients,sum(bit(x) for x in source['ingredients']))
            self.assertEqual(item.recipe.main,sum(bit(x) for x in source['ingredients'] if x.get('main')))
            self.assertEqual(item.recipe.courses,sum(1<<dictionary['courses'].index(x) for x in source['course']))
            self.assertEqual(item.season,sum(1<<(x-1) for x in source.get('season',[])))
            self.assertEqual(item.servings,source['servings'])
            dish=Dish();self.assertEqual(self.dll.kc_read_dish(C.byref(pack),i,C.byref(dish)),1)
            self.assertEqual(dictionary['vessels'][dish.vessel],source['dish']['vessel'])
            self.assertEqual(None if dish.sauce<0 else dictionary['sauces'][dish.sauce],source['dish']['sauce'])
            self.assertEqual([dictionary['assets'][x] for x in dish.layers[:dish.layer_count]],source['dish']['layers'])
            self.assertEqual([dictionary['garnishes'][x] for x in dish.garnishes[:dish.garnish_count]],source['dish'].get('garnish',[]))
            listed=(Ingredient*10)();amount=C.c_uint()
            self.assertEqual(self.dll.kc_read_ingredients(C.byref(pack),i,listed,10,C.byref(amount)),1)
            self.assertEqual(amount.value,len(source['ingredients']))
            units=['克','个','根','瓣','块','片','条','只','把','颗','朵','段','斤','适量']
            for row,wanted in zip(listed,source['ingredients']):
                self.assertEqual((dictionary['ingredients'][row.id],row.qty100,units[row.unit],bool(row.main)),
                                 (wanted['id'],wanted['qty']*100,wanted['unit'],wanted.get('main',False)))
            for step_no,expected_step in enumerate(source['steps']):
                step=Step();count=C.c_uint()
                self.assertEqual(self.dll.kc_read_step(C.byref(pack),i,step_no,C.byref(step),C.byref(count)),1)
                self.assertEqual(count.value,len(source['steps']))
                self.assertEqual(step.text.decode(),expected_step['text'])
                self.assertEqual(step.tip.decode(),expected_step.get('tip',''))
                self.assertEqual(step.timer,expected_step.get('timer',0))
                self.assertEqual(dictionary['methods'][step.method],expected_step['method'])
                self.assertEqual(dictionary['heats'][step.heat],expected_step['heat'])
            self.assertEqual(self.dll.kc_read_step(C.byref(pack),i,len(source['steps']),C.byref(Step()),None),0)
        catalog.capacity=pack.count-1
        self.assertEqual(self.dll.kc_load(C.byref(catalog),C.byref(pack)),0)
        self.assertEqual(catalog.count,0)
if selected():
    class CatalogReaderSelected(CatalogReader):
        recipe_path,metadata_path=selected()
        complete=True
if __name__=='__main__':unittest.main()
