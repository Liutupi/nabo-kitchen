"""The four offline home-menu icons must be reproducible and decode at device size."""
import csv
import hashlib
from io import BytesIO
import json
from pathlib import Path
import sys
import unittest

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
import _bootstrap  # noqa: F401
from asset_codec import index_image, pack_sprites, prepare_pil, unpack_sprites
from generate_kitchen_menu_icons import icon
from pack_assets import profile

IDS=('ui_fridge','ui_shopping','ui_recipe_book','ui_settings')
class MenuIconTest(unittest.TestCase):
    def test_source_images_and_review_metadata_are_reproducible(self):
        with (ROOT/'game-art/kitchen/ASSET_LIST.csv').open(encoding='utf-8-sig',newline='') as stream:
            rows={r['id']:r for r in csv.DictReader(stream)}
        hashes=set()
        for ident in IDS:
            with self.subTest(ident=ident):
                image=icon(ident);out=BytesIO();image.save(out,format='PNG',optimize=False)
                data=out.getvalue();path=ROOT/'game-art/kitchen/icons'/f'{ident}.png'
                metadata=json.loads(path.with_suffix('.json').read_text('utf8'))
                digest=hashlib.sha256(data).hexdigest();hashes.add(digest)
                self.assertEqual(data,path.read_bytes());self.assertEqual(metadata['sha256'],digest)
                self.assertEqual(metadata['status'],'review');self.assertEqual(rows[ident]['status'],'review')
                self.assertEqual(image.size,(160,160));self.assertEqual(image.getchannel('A').getextrema(),(0,255))
                self.assertEqual(image.getpixel((0,0))[3],0)
                self.assertEqual(profile(rows[ident]),('icons','menu-icon',(40,40),False,False))
        self.assertEqual(len(hashes),len(IDS))
    def test_indexed_sprite_pack_roundtrips_all_icons(self):
        entries=[{'id':ident,'group':'menu-icon','prepared':prepare_pil(icon(ident),(40,40))} for ident in IDS]
        packed=pack_sprites(entries);decoded=unpack_sprites(packed)
        self.assertEqual(set(decoded.assets),set(IDS))
        for entry in entries:
            self.assertEqual(decoded.assets[entry['id']]['frame'],(40,40))
            self.assertEqual(decoded.assets[entry['id']]['indices'],index_image(entry['prepared'],decoded.groups['menu-icon']))

if __name__=='__main__':unittest.main()
