"""Draw eight original seafood inventory icons on transparent canvases.

The drawing source is reproducible; the 640px original and the 160px runtime
source are retained separately. Colors and facial details follow ART_BRIEF §7.
"""
import argparse
from pathlib import Path
import csv
import hashlib
import json
from io import BytesIO
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'game-art/kitchen/ingredients'
ORIGINALS = OUT / 'originals'
CSV = ROOT / 'game-art/kitchen/ASSET_LIST.csv'
S = 4
INK = '#3B2730'
BLUSH = '#E98786'
CREAM = '#FAEAD5'

ITEMS = {
    'crab': '整蟹',
    'scallop': '去壳扇贝肉',
    'oyster_meat': '去壳牡蛎肉',
    'mussel': '带壳青口贝',
    'razor_clam': '带壳蛏子',
    'abalone': '去壳鲍鱼肉',
    'octopus': '章鱼',
    'salmon': '生三文鱼肉',
}


class Art:
    def __init__(self):
        self.image = Image.new('RGBA', (160*S, 160*S), (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.image)

    @staticmethod
    def points(pts):
        return [(round(x*S), round(y*S)) for x, y in pts]

    @staticmethod
    def box(b):
        return tuple(round(v*S) for v in b)

    def ellipse(self, box, fill, outline=INK, width=4):
        self.d.ellipse(self.box(box), fill=fill, outline=outline, width=width*S)

    def polygon(self, pts, fill, outline=INK, width=4):
        p = self.points(pts)
        self.d.polygon(p, fill=fill)
        self.d.line(p + [p[0]], fill=outline, width=width*S, joint='curve')

    def smooth(self, pts, fill, outline=INK, width=4):
        # Closed Catmull-Rom spline: a soft edible silhouette, not a gear.
        n = len(pts)
        samples = []
        for i in range(n):
            p0, p1, p2, p3 = pts[(i-1)%n], pts[i], pts[(i+1)%n], pts[(i+2)%n]
            for j in range(8):
                t = j / 8
                t2, t3 = t*t, t*t*t
                samples.append(tuple(.5*((2*p1[k]) + (-p0[k]+p2[k])*t +
                                  (2*p0[k]-5*p1[k]+4*p2[k]-p3[k])*t2 +
                                  (-p0[k]+3*p1[k]-3*p2[k]+p3[k])*t3) for k in (0,1)))
        self.polygon(samples, fill, outline, width)

    def line(self, pts, fill=INK, width=4):
        self.d.line(self.points(pts), fill=fill, width=width*S, joint='curve')

    def arc(self, box, start, end, fill=INK, width=3):
        self.d.arc(self.box(box), start=start, end=end, fill=fill, width=width*S)

    def face(self, x, y, scale=1):
        # Face includes two bead eyes, two blush marks and a small smile.
        for dx in (-11, 11):
            self.ellipse((x + (dx-2)*scale, y-3*scale, x+(dx+2)*scale, y+3*scale),
                         INK, INK, 1)
        for dx in (-18, 18):
            self.ellipse((x+(dx-4)*scale, y+4*scale, x+(dx+4)*scale, y+8*scale),
                         BLUSH, BLUSH, 1)
        self.arc((x-5*scale, y+3*scale, x+5*scale, y+10*scale), 10, 170, INK, 2)


def draw_icon(ident):
    a = Art()
    if ident == 'crab':
        # A whole crab with paired claws and three visible walking legs per side.
        for sign in (-1, 1):
            for y in (91, 104, 117):
                a.line([(80+sign*45, y-7), (80+sign*57, y), (80+sign*61, y+8)], INK, 6)
                a.line([(80+sign*45, y-7), (80+sign*57, y)], '#E96E5B', 3)
            a.line([(80+sign*43, 73), (80+sign*58, 63)], INK, 9)
            a.ellipse((80+sign*54-10, 42, 80+sign*54+10, 68), '#E87560')
            a.polygon([(80+sign*51, 42), (80+sign*57, 33), (80+sign*60, 49)], '#F58D73')
        a.ellipse((31, 62, 129, 125), '#E87763', width=5)
        a.ellipse((46, 69, 113, 92), '#F59D82', outline='#F59D82', width=1)
        a.face(80, 100, .8)
    elif ident == 'scallop':
        # Removed from the shell: a round ivory adductor muscle, not a fan shell.
        a.ellipse((34, 34, 126, 126), '#D8BA9E', width=5)
        a.ellipse((38, 38, 122, 115), '#F6E6CF', width=3)
        a.arc((48, 46, 110, 82), 190, 338, '#FFF8E9', 6)
        a.arc((48, 83, 110, 110), 15, 165, '#D6B294', 4)
        a.face(80, 78, .85)
    elif ident == 'oyster_meat':
        # Ruffled raw meat with a pale center, without an oyster shell.
        a.smooth([(30,65),(37,46),(54,37),(69,31),(85,37),(105,34),(122,48),
                  (126,67),(133,80),(124,96),(119,113),(101,117),(86,125),
                  (68,120),(51,123),(35,110),(27,94),(24,78)], '#8E7F79', width=5)
        a.ellipse((42, 43, 116, 110), '#EAD7C4', width=3)
        a.ellipse((51, 48, 92, 59), '#FFF3E5', outline='#FFF3E5', width=1)
        a.face(79, 76, .8)
    elif ident == 'mussel':
        # Live, closed blue-black mussel shell; asymmetric pointed end.
        a.polygon([(32,25),(57,29),(84,40),(111,60),(127,87),(124,113),
                   (112,131),(94,135),(68,126),(46,107),(33,80),(27,52)], '#475B68', width=5)
        a.line([(33,29),(55,48),(75,68),(91,91),(107,119)], '#92A8AE', 5)
        a.line([(40,34),(58,46),(71,58)], '#C2CFCC', 3)
        a.face(82, 92, .75)
    elif ident == 'razor_clam':
        # Long narrow shell with rounded tips, clearly unlike a round clam.
        a.smooth([(22,67),(28,54),(43,49),(118,45),(133,51),(138,65),
                  (134,82),(122,91),(43,98),(26,93),(20,80)], '#C9AA8E', width=5)
        a.smooth([(27,64),(36,56),(117,51),(130,56),(132,68),(28,76)], '#EED7BB', width=2)
        a.line([(31,80),(128,73)], '#9D7F70', 3)
        a.face(80, 82, .7)
    elif ident == 'abalone':
        # Removed from shell: plump oval foot with dark ruffled edge, no shell holes.
        a.smooth([(34,48),(45,36),(62,31),(82,33),(101,29),(118,40),(128,56),
                  (127,73),(133,87),(126,105),(115,118),(98,124),(82,121),
                  (64,126),(46,118),(34,105),(28,87),(31,68)], '#8D7468', width=5)
        a.ellipse((40, 39, 122, 115), '#D9C2A7', width=3)
        a.ellipse((49, 46, 102, 62), '#F4E3CD', outline='#F4E3CD', width=1)
        a.arc((53, 76, 109, 109), 15, 155, '#B79580', 3)
        a.face(81, 79, .8)
    elif ident == 'octopus':
        # Rounded head and short curled arms, distinct from the existing squid.
        for pts in [[(56,92),(44,106),(30,107),(27,99)],[(63,100),(58,119),(47,128)],
                    [(98,92),(112,106),(127,106),(130,97)],[(91,100),(97,119),(109,127)]]:
            a.line(pts, INK, 14); a.line(pts, '#CC8791', 7)
        a.ellipse((42, 26, 118, 108), '#D896A0', width=5)
        a.ellipse((52, 35, 89, 48), '#F2B9B5', outline='#F2B9B5', width=1)
        a.face(80, 75, .9)
    elif ident == 'salmon':
        # A raw fillet with cream fat stripes and a small skin edge, not a whole fish.
        a.polygon([(25,48),(39,32),(115,36),(133,50),(130,105),(116,125),
                   (43,123),(25,107)], '#ED9179', width=5)
        a.polygon([(27,102),(43,113),(128,100),(125,112),(115,125),(43,123),(26,108)],
                  '#60717A', width=2)
        for x in (44, 65, 87, 109):
            a.line([(x,43),(x-9,63),(x-14,85),(x-16,103)], '#FFE4CD', 6)
        a.face(81, 78, .8)
    else:
        raise ValueError(ident)
    return a.image


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--update', action='store_true', help='Regenerate only unchanged files from this generator')
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    ORIGINALS.mkdir(parents=True, exist_ok=True)
    with CSV.open(encoding='utf-8-sig', newline='') as f:
        existing = {row['id'] for row in csv.DictReader(f)}
    if existing.intersection(ITEMS) and not args.update:
        raise SystemExit('Refusing to overwrite registered assets; inspect the CSV first')
    if args.update and not set(ITEMS) <= existing:
        raise SystemExit('Cannot update a partial or unregistered set')
    if args.update:
        for ident in ITEMS:
            source, target, meta = ORIGINALS / f'{ident}-source.png', OUT / f'{ident}.png', OUT / f'{ident}.json'
            payload = json.loads(meta.read_text(encoding='utf-8'))
            if (payload.get('source_script') != 'tools/generate_seafood_icons.py' or
                payload.get('original_sha256') != hashlib.sha256(source.read_bytes()).hexdigest() or
                payload.get('sha256') != hashlib.sha256(target.read_bytes()).hexdigest()):
                raise SystemExit(f'Refusing to overwrite modified asset {ident}')
    for ident, subject in ITEMS.items():
        original = draw_icon(ident)
        source = ORIGINALS / f'{ident}-source.png'
        target = OUT / f'{ident}.png'
        meta = OUT / f'{ident}.json'
        if not args.update and (source.exists() or target.exists() or meta.exists()):
            raise SystemExit(f'Refusing to overwrite existing {ident}')
        original.save(source, optimize=True)
        image = original.resize((160, 160), Image.Resampling.LANCZOS)
        image.save(target, optimize=True)
        payload = {
            'id': ident, 'subject': subject, 'status': 'review',
            'generator': 'original deterministic vector drawing via Pillow',
            'source_script': 'tools/generate_seafood_icons.py',
            'original': f'ingredients/originals/{source.name}',
            'original_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
            'path': f'ingredients/{target.name}', 'size': [160, 160],
            'sha256': hashlib.sha256(target.read_bytes()).hexdigest(),
            'note': 'Pending visual and device acceptance; no photo or brand imagery.'
        }
        meta.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    if not args.update:
        with CSV.open('a', encoding='utf-8', newline='') as f:
            w = csv.writer(f, lineterminator='\n')
            for ident, subject in ITEMS.items():
                w.writerow([ident, 'ingredient', f'ingredients/{ident}.png', '40x40', '160x160',
                            '0.3', 'Codex', 'review', f'{subject}；原创图标，待审图及实机验收'])
    print('Created 8 review icons and their reproducible 640px sources')


if __name__ == '__main__':
    main()
