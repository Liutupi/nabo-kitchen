"""Render code-native release posters using exact UI captures and dish artwork.

Requires Pillow, fonttools, a native CC/CJSON_DIR, and Node.js with sharp.
The AI-generated cover is a separate original; no screenshot is AI-redrawn.
"""
import argparse
import base64
import ctypes as ct
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess

from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'assets/community/v0.1.0'
NAVY, PANEL, PAPER, INK, CORAL, GOLD = '#19162F', '#2C2845', '#FBF1E3', '#3B2730', '#FF6655', '#FFBA58'


class Poster:
    def __init__(self, font, dark=True):
        self.font = font
        self.glyphs = font.getGlyphSet()
        self.cmap = font.getBestCmap()
        self.units = font['head'].unitsPerEm
        self.dark = dark
        self.parts = [f'<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="900" height="1200" viewBox="0 0 900 1200"><rect width="900" height="1200" fill="{NAVY if dark else PAPER}"/>']
        self.parts.append(f'<circle cx="850" cy="0" r="360" fill="{PANEL if dark else "#F7E6CB"}"/>')
        self.text('NABO  /  MIDNIGHT KITCHEN', 56, 53, 17, GOLD if dark else INK)
        self.text('v0.1.0', 740, 53, 17, GOLD if dark else INK)
        self.parts.append(f'<rect x="56" y="78" width="60" height="6" rx="3" fill="{CORAL}"/>')

    def rect(self, x, y, w, h, fill, radius=22, stroke=None):
        self.parts.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{radius}" fill="{fill}"' + (f' stroke="{stroke}" stroke-width="3"' if stroke else '') + '/>')

    def text(self, value, x, y, size, color=None, max_width=800):
        color = color or (PAPER if self.dark else INK)
        cursor, paths = 0, []
        for char in value:
            name = self.cmap.get(ord(char))
            if name is None:
                raise ValueError('Missing font glyph: ' + char)
            pen = SVGPathPen(self.glyphs)
            self.glyphs[name].draw(pen)
            path = pen.getCommands()
            if path:
                paths.append(f'<path transform="translate({cursor} 0)" d="{path}"/>')
            cursor += self.font['hmtx'][name][0]
        assert cursor * size / self.units <= max_width, value
        self.parts.append(f'<g fill="{color}" transform="translate({x} {y}) scale({size / self.units} {-size / self.units})">' + ''.join(paths) + '</g>')

    def image(self, path, x, y, w, h):
        encoded = base64.b64encode(path.read_bytes()).decode('ascii')
        self.parts.append(f'<image x="{x}" y="{y}" width="{w}" height="{h}" preserveAspectRatio="xMidYMid meet" xlink:href="data:image/png;base64,{encoded}"/>')

    def footer(self, first, second):
        self.rect(56, 1096, 788, 2, GOLD if self.dark else '#DBB98A', 0)
        self.text(first, 56, 1133, 19)
        self.text(second, 56, 1166, 16, '#B5AFC6' if self.dark else '#7B6760')

    def save(self, name):
        (OUT / (name + '.svg')).write_text(''.join(self.parts) + '</svg>', encoding='utf-8')


def render_ui():
    spec = importlib.util.spec_from_file_location('release_media_views', ROOT / 'tests/test_real_catalog_views.py')
    views = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(views)
    cls = views.RealCatalogViews
    cls.setUpClass()
    try:
        for page, name in [(1, 'home'), (2, 'conditions'), (5, 'ingredients'), (6, 'steps'), (7, 'timer')]:
            assert cls.dll.view_page(page, cls.ids.index('fanqie-chaodan')) == 1
            pixels = (ct.c_uint16 * (240 * 320))()
            assert cls.dll.view_draw(pixels, 0, 320) == 1
            frame = Image.new('RGB', (240, 320))
            frame.putdata([(((v >> 11) * 255 + 15) // 31, (((v >> 5) & 63) * 255 + 31) // 63, ((v & 31) * 255 + 15) // 31) for v in pixels])
            frame.save(OUT / 'source-ui' / (name + '.png'))
    finally:
        cls.tearDownClass()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--content-root', required=True, type=Path)
    parser.add_argument('--font', required=True, type=Path)
    parser.add_argument('--sharp-root', required=True, type=Path)
    args = parser.parse_args()
    for folder in ('source-ui', 'source-art'):
        (OUT / folder).mkdir(parents=True, exist_ok=True)
    hero = args.content_root / 'game-art/kitchen/dish-hero'
    source_batch = hero / 'batches/2026-10-29'
    foods = [('fanqie-chaodan', '番茄炒蛋'), ('qingzheng-luyu', '清蒸鲈鱼'), ('baiqie-ji', '白切鸡'),
             ('jidan-chaofan', '鸡蛋炒饭'), ('yumi-paigu-tang', '玉米排骨汤'), ('pai-huanggua', '拍黄瓜')]
    for ident, _ in foods:
        shutil.copyfile(hero / ('hero_' + ident + '.png'), OUT / 'source-art' / ('hero_' + ident + '.png'))
    for n in range(1, 7):
        shutil.copyfile(source_batch / 'device-captures' / ('capture-%02d.png' % n), OUT / 'source-ui' / ('device-%02d.png' % n))
    render_ui()
    font = TTFont(args.font, fontNumber=0)

    p = Poster(font, False)
    p.text('每一道，', 56, 172, 68)
    p.text('都看得见香气。', 56, 253, 68)
    p.text('144 道家常菜谱，选一道试试。', 58, 302, 27)
    for n, (ident, label) in enumerate(foods):
        x, y = 56 + (n % 2) * 404, 342 + (n // 2) * 230
        p.rect(x, y, 384, 212, '#FFFFFF', 24, '#E5D4BD')
        p.image(OUT / 'source-art' / ('hero_' + ident + '.png'), x + 42, y + 5, 300, 158)
        p.text(label, x + 28, y + 190, 24)
    for x, number, label in [(56, '144', '道菜谱'), (333, '58', '种食材'), (610, '13', '种海鲜')]:
        p.text(number, x, 1070, 42, CORAL)
        p.text(label, x + 105, 1070, 22)
    p.footer('一机离线，三键操作。', '菜品插画排版 · 首版菜谱与插画仍在持续校对')
    p.save('02-dish-collection')

    p = Poster(font)
    p.text('今晚吃什么，', 56, 175, 65)
    p.text('交给 NABO。', 56, 253, 65)
    p.text('先选人数和时间，不合口味就继续换。', 58, 315, 27)
    for x, path, label, detail in [(56, 'conditions.png', '01  选好条件', '几个人吃，多少时间'),
                                   (478, 'device-04.png', '02  看见答案', '不满意，就换一道')]:
        p.rect(x, 365, 366, 548, '#3B314B')
        p.image(OUT / 'source-ui' / path, x + 15, 380, 336, 448)
        p.text(label, x + 22, 868, 28)
        p.text(detail, x + 22, 900, 19, '#C6BFD1')
    p.rect(56, 953, 788, 88, PAPER)
    p.text('从一道菜，到荤素搭配。', 84, 1009, 34, INK)
    p.footer('不需要配网，开机按 OK 就能开始。', '左图：本版代码绘制 · 右图：设备采集，保留 TEST 标记')
    p.save('03-tonights-menu')

    p = Poster(font, False)
    p.text('从用料，', 56, 172, 68)
    p.text('陪你到出锅。', 56, 253, 68)
    p.text('有步骤，也有提醒；下厨慢慢来。', 58, 315, 27)
    for x, path, label in [(56, 'ingredients.png', '用料一目了然'), (478, 'steps.png', '照着步骤慢慢做')]:
        p.rect(x, 365, 366, 548, '#E5D4BD')
        p.image(OUT / 'source-ui' / path, x + 15, 380, 336, 448)
        p.text(label, x + 22, 881, 28)
    for x, label in [(56, '用料到采购'), (326, '步骤跟做'), (596, '计时可暂停')]:
        p.rect(x, 953, 248, 74, NAVY)
        p.text(label, x + 25, 1001, 26, PAPER)
    p.footer('冰箱食材、采购清单、做菜记录，随手查看。', '界面由本版生产代码与资源绘制 · 非拍摄照片')
    p.save('04-cook-together')

    p = Poster(font)
    p.text('六道菜，', 56, 170, 65)
    p.text('真实设备画面。', 56, 246, 65)
    labels = ['蒸水蛋', '玉米排骨汤', '白菜炖豆腐', '番茄炒蛋', '青椒蛏子汤', '清蒸鲈鱼']
    for n, label in enumerate(labels):
        x, y = 56 + (n % 3) * 274, 300 + (n // 3) * 390
        p.rect(x - 3, y - 3, 246, 326, '#65526B', 10)
        p.image(OUT / 'source-ui' / ('device-%02d.png' % (n + 1)), x, y, 240, 320)
        p.text(label, x + 4, y + 359, 22)
    p.footer('来自工牌完整绘制画面，未补画或拼接缺行。', 'TEST 是隔离验证标记，日常使用不显示该标记')
    p.save('05-device-captures')

    runner = OUT / 'render-svg.cjs'
    runner.write_text("const fs=require('fs'),path=require('path'),sharp=require(process.argv[2]);(async()=>{for(const name of process.argv.slice(3)){const p=path.join(__dirname,name);await sharp(p+'.svg').webp({quality:96}).toFile(p+'.webp');console.log(name)}})().catch(e=>{console.error(e);process.exit(1)})", encoding='utf-8')
    names = ['02-dish-collection', '03-tonights-menu', '04-cook-together', '05-device-captures']
    subprocess.run(['node', str(runner), str(args.sharp_root), *names], check=True)
    files = {str(path.relative_to(OUT)).replace('\\', '/'): {'bytes': path.stat().st_size,
             'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
             for path in sorted(OUT.rglob('*')) if path.is_file() and path.name != 'media-manifest.json'}
    (OUT / 'media-manifest.json').write_text(json.dumps({'version': 'v0.1.0', 'cover': 'built-in OpenAI imagegen; promotional illustration',
        'gallery': 'native SVG layout; exact project dish PNGs, production C rendering and unmodified device captures',
        'font_source_sha256': hashlib.sha256(args.font.read_bytes()).hexdigest(), 'files': files}, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
