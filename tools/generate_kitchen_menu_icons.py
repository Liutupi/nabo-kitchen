"""Generate original, transparent 160px menu icons in NABO's flat outlined style."""
from io import BytesIO
from pathlib import Path
import hashlib
import json
import math

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT/'game-art/kitchen/icons'
INK = '#3B2730'
CREAM = '#FBF1E3'
GOLD = '#FFB45E'
RED = '#FF6B5B'
BLUE = '#8EB4CF'
GREEN = '#8CC1A3'

def icon(kind):
    image = Image.new('RGBA', (160, 160), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    if kind == 'ui_fridge':
        draw.rounded_rectangle((30, 14, 130, 145), radius=18, fill=CREAM, outline=INK, width=9)
        draw.line((34, 78, 126, 78), fill=INK, width=8)
        draw.rounded_rectangle((106, 32, 115, 58), radius=4, fill=INK)
        draw.rounded_rectangle((106, 94, 115, 119), radius=4, fill=INK)
        draw.ellipse((49, 32, 68, 51), fill=RED, outline=INK, width=4)
        draw.line((48, 142, 48, 151), fill=INK, width=8)
        draw.line((112, 142, 112, 151), fill=INK, width=8)
    elif kind == 'ui_shopping':
        draw.line([(48, 65), (60, 36), (67, 30)], fill=INK, width=10, joint='curve')
        draw.line([(112, 65), (100, 36), (93, 30)], fill=INK, width=10, joint='curve')
        draw.line([(48, 65), (60, 36), (67, 30)], fill=GOLD, width=4, joint='curve')
        draw.line([(112, 65), (100, 36), (93, 30)], fill=GOLD, width=4, joint='curve')
        draw.polygon([(17, 57), (143, 57), (129, 137), (31, 137)], fill=GOLD, outline=INK, width=9)
        draw.rounded_rectangle((14, 53, 146, 71), radius=8, fill=CREAM, outline=INK, width=8)
        draw.line((55, 85, 58, 116), fill=INK, width=7)
        draw.line((80, 85, 80, 116), fill=INK, width=7)
        draw.line((105, 85, 102, 116), fill=INK, width=7)
    elif kind == 'ui_recipe_book':
        draw.rounded_rectangle((26, 18, 133, 145), radius=13, fill=BLUE, outline=INK, width=9)
        draw.rounded_rectangle((46, 26, 124, 136), radius=5, fill=CREAM, outline=INK, width=6)
        draw.line((36, 27, 36, 136), fill=INK, width=7)
        draw.polygon([(94, 25), (113, 25), (113, 62), (103, 55), (94, 62)], fill=BLUE, outline=INK, width=4)
        draw.ellipse((61, 62, 99, 100), fill=BLUE, outline=INK, width=5)
        draw.line((61, 112, 108, 112), fill=INK, width=6)
    elif kind == 'ui_settings':
        points=[]
        for i in range(32):
            radius = 65 if i % 4 in (1, 2) else 52
            angle = -math.pi / 2 + math.pi * 2 * i / 32
            points.append((round(80 + radius * math.cos(angle)), round(80 + radius * math.sin(angle))))
        draw.polygon(points, fill=GREEN, outline=INK, width=9)
        draw.ellipse((50, 50, 110, 110), fill=CREAM, outline=INK, width=8)
        draw.ellipse((72, 72, 88, 88), fill=GREEN, outline=INK, width=4)
    else:
        raise ValueError('Unknown icon')
    return image

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    for ident in ('ui_fridge','ui_shopping','ui_recipe_book','ui_settings'):
        image=icon(ident)
        binary=BytesIO();image.save(binary, format='PNG', optimize=False);data=binary.getvalue()
        dest=OUT/(ident+'.png')
        if dest.exists() and dest.read_bytes()!=data:
            raise RuntimeError(f'Refusing to overwrite different artwork: {dest}')
        dest.write_bytes(data)
        metadata={'id':ident,'status':'review','generator':'original repository-native geometry, tools/generate_kitchen_menu_icons.py',
                  'size':[160,160],'palette':[INK,CREAM,'one feature accent'],'sha256':hashlib.sha256(data).hexdigest(),
                  'notes':'Unaccepted candidate; target-size and light/dark backgrounds require visual review.'}
        meta_path=OUT/(ident+'.json')
        meta_text=json.dumps(metadata,ensure_ascii=False,indent=2)+'\n'
        if meta_path.exists() and meta_path.read_text(encoding='utf8') != meta_text:
            raise RuntimeError(f'Refusing to overwrite different metadata: {meta_path}')
        meta_path.write_text(meta_text,encoding='utf8')
        print(ident, len(data), metadata['sha256'])
if __name__=='__main__':main()
