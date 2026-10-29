"""Measure 1bpp alternatives from the frozen KFN1, without replacing it."""
from dataclasses import replace
from pathlib import Path
import sys
import zlib

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from font_codec import coverage_bytes, coverage_values, pack_fonts, unpack_fonts

blob = (ROOT / 'checkpoints/2026-10-28/packages/kitchen_fonts.bin').read_bytes()
source = unpack_fonts(blob)
print(f'2bpp KFN1: {len(blob)} bytes; whole-file zlib-9: {len(zlib.compress(blob, 9))} bytes')
for mode in ('body', 'all'):
    fonts = []
    for name, font in source.fonts.items():
        convert = mode == 'all' or name in ('han12', 'han16', 'han18')
        glyphs = []
        for glyph in font['glyphs'].values():
            if convert:
                values = coverage_values(glyph.coverage, glyph.width * glyph.height, font['bpp'])
                glyph = replace(glyph, coverage=coverage_bytes([int(v >= 2) for v in values], 1))
            glyphs.append(glyph)
        fonts.append(dict(id=name, px=font['px'], bpp=1 if convert else font['bpp'],
                          ascent=font['ascent'], descent=font['descent'],
                          line_height=font['line_height'], glyphs=glyphs))
    candidate = pack_fonts(fonts)
    decoded = unpack_fonts(candidate)
    assert sum(len(f['glyphs']) for f in decoded.fonts.values()) == 12447
    print(f'{mode} 1bpp KFN1: {len(candidate)} bytes; 380000-byte budget: '
          f'{"PASS" if len(candidate) <= 380000 else "FAIL"}')
