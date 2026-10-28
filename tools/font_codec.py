"""KFN1 small bitmap fonts: integer glyph metrics, 1/2bpp and bounded RLE."""
from dataclasses import dataclass
import binascii
import re
import struct
import _bootstrap
from PIL import Image
from asset_codec import packbits_encode, packbits_decode

HEADER = struct.Struct("<4sHH7I")
FONT = struct.Struct("<16sHHIIhhHH")
GLYPH = struct.Struct("<IBBbbBBIH")
MAX_GLYPHS = 100000
SAFE_NAME = re.compile(r"^[a-z][a-z0-9_-]{0,14}$")


def coverage_bytes(values, bpp):
    if bpp not in (1, 2):
        raise ValueError("font bpp must be1 or2")
    values = list(values)
    maximum = (1 << bpp) - 1
    out = bytearray((len(values) * bpp + 7) // 8)
    for n, value in enumerate(values):
        if not 0 <= value <= maximum:
            raise ValueError("coverage outside bpp")
        out[n * bpp // 8] |= value << (8 - bpp - n * bpp % 8)
    return bytes(out)


def coverage_values(data, count, bpp):
    if bpp not in (1, 2) or len(data) != (count * bpp + 7) // 8:
        raise ValueError("invalid coverage length or bpp")
    if count * bpp % 8 and data and data[-1] & ((1 << (8 - count * bpp % 8)) - 1):
        raise ValueError("nonzero coverage padding")
    mask = (1 << bpp) - 1
    return [(data[n * bpp // 8] >> (8 - bpp - n * bpp % 8)) & mask for n in range(count)]


@dataclass(frozen=True)
class Glyph:
    cp: int
    width: int
    height: int
    x: int
    y: int
    advance: int
    coverage: bytes

    def image(self, bpp=2):
        values = coverage_values(self.coverage, self.width * self.height, bpp)
        scale = 255 // ((1 << bpp) - 1)
        return Image.frombytes("L", (self.width, self.height), bytes(v * scale for v in values))


@dataclass
class FontPack:
    fonts: dict

    def glyph(self, name, cp):
        return self.fonts[name]["glyphs"][cp]


def pack_fonts(fonts):
    if not fonts or len(fonts) > 32:
        raise ValueError("font count outside bounds")
    names, glyph_total = set(), 0
    for font in fonts:
        name = font["id"]
        if not SAFE_NAME.fullmatch(name) or name in names:
            raise ValueError("invalid or duplicate font name")
        names.add(name)
        if font["bpp"] not in (1, 2) or not 1 <= font["px"] <= 64:
            raise ValueError("font size/bpp outside bounds")
        glyph_total += len(font["glyphs"])
    if glyph_total > MAX_GLYPHS:
        raise ValueError("too many glyphs")
    font_off = HEADER.size
    glyph_off = font_off + FONT.size * len(fonts)
    payload_off = glyph_off + GLYPH.size * glyph_total
    font_data, index, payload = bytearray(), bytearray(), bytearray()
    cache, at = {}, 0
    for font in fonts:
        glyphs = sorted(font["glyphs"], key=lambda g: g.cp)
        if len({g.cp for g in glyphs}) != len(glyphs):
            raise ValueError("duplicate codepoint")
        font_data.extend(FONT.pack(font["id"].encode(), font["px"], font["bpp"], at, len(glyphs),
                                   font["ascent"], font["descent"], font["line_height"], 0))
        for g in glyphs:
            if not 0 <= g.cp <= 0x10ffff or 0xd800 <= g.cp <= 0xdfff:
                raise ValueError("invalid unicode scalar")
            if not (0 <= g.width <= 64 and 0 <= g.height <= 64 and -128 <= g.x <= 127
                    and -128 <= g.y <= 127 and 0 <= g.advance <= 128):
                raise ValueError("glyph metrics outside bounds")
            coverage_values(g.coverage, g.width * g.height, font["bpp"])
            encoded = packbits_encode(g.coverage)
            codec = int(len(encoded) < len(g.coverage))
            encoded = encoded if codec else g.coverage
            key = (codec, encoded)
            if key not in cache:
                cache[key] = payload_off + len(payload)
                payload.extend(encoded)
            offset = cache[key]
            index.extend(GLYPH.pack(g.cp, g.width, g.height, g.x, g.y, g.advance,
                                    codec, offset, len(encoded)))
        at += len(glyphs)
    body = bytes(font_data + index + payload)
    return HEADER.pack(b"KFN1", 1, GLYPH.size, len(fonts), glyph_total, font_off, glyph_off,
                       payload_off, HEADER.size + len(body), binascii.crc32(body)) + body


def unpack_fonts(blob):
    if len(blob) < HEADER.size:
        raise ValueError("truncated KFN1 header")
    magic, version, entry_size, count, total, font_off, glyph_off, payload_off, length, crc = HEADER.unpack_from(blob)
    if magic != b"KFN1" or version != 1 or entry_size != GLYPH.size:
        raise ValueError("unknown KFN1 version")
    if not 1 <= count <= 32 or total > MAX_GLYPHS or length != len(blob):
        raise ValueError("invalid KFN1 count/length")
    if (font_off != HEADER.size or glyph_off != font_off + count * FONT.size
            or payload_off != glyph_off + total * GLYPH.size or payload_off > length):
        raise ValueError("invalid KFN1 offsets")
    if binascii.crc32(blob[HEADER.size:]) != crc:
        raise ValueError("KFN1 checksum mismatch")
    fonts, next_index = {}, 0
    for n in range(count):
        name, px, bpp, start, size, ascent, descent, line_height, reserved = FONT.unpack_from(blob, font_off + n * FONT.size)
        try:
            name = name.rstrip(b"\0").decode("ascii")
        except UnicodeDecodeError as error:
            raise ValueError("invalid font ID") from error
        if (not SAFE_NAME.fullmatch(name) or name in fonts or not 1 <= px <= 64
                or bpp not in (1, 2) or start != next_index or start + size > total
                or reserved or ascent < 0 or descent < 0 or not px <= line_height <= 256):
            raise ValueError("invalid font directory")
        glyphs, previous_cp = {}, -1
        for at in range(start, start + size):
            cp, w, h, x, y, advance, codec, offset, encoded_len = GLYPH.unpack_from(blob, glyph_off + at * GLYPH.size)
            if (cp <= previous_cp or cp > 0x10ffff or 0xd800 <= cp <= 0xdfff
                    or w > 64 or h > 64 or advance > 128 or codec not in (0, 1)
                    or offset < payload_off or offset + encoded_len > length):
                raise ValueError("invalid glyph index")
            data = blob[offset:offset + encoded_len]
            expected = (w * h * bpp + 7) // 8
            data = packbits_decode(data, expected) if codec else data
            coverage_values(data, w * h, bpp)
            glyphs[cp] = Glyph(cp, w, h, x, y, advance, data)
            previous_cp = cp
        next_index = start + size
        fonts[name] = dict(px=px, bpp=bpp, ascent=ascent, descent=descent,
                           line_height=line_height, glyphs=glyphs)
    if next_index != total:
        raise ValueError("unreferenced glyph index")
    return FontPack(fonts)
