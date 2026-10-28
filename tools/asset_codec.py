"""KSP1 indexed RGB565 sprites, shared image preparation and bounded PackBits."""
import binascii
from collections import defaultdict
from dataclasses import dataclass
import struct
import re

import _bootstrap  # noqa: F401
from PIL import Image, ImageChops, ImageFilter

OUTLINE = (59, 39, 48)
HEADER = struct.Struct('<4sHH8I')
GROUP = struct.Struct('<IHHII16s')
ENTRY = struct.Struct('<II8HIII')
MAX_PIXELS = 1024 * 1024
SAFE_NAME = re.compile(r'^[A-Za-z0-9_-]{1,128}$')


def fnv1a(text):
    value = 2166136261
    for byte in text.encode('utf-8'):
        value = ((value ^ byte) * 16777619) & 0xffffffff
    return value


def rgb565(rgb):
    r, g, b = rgb
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def expand565(word):
    return (((word >> 11) * 255 + 15) // 31,
            (((word >> 5) & 63) * 255 + 31) // 63,
            ((word & 31) * 255 + 15) // 31)


def packbits_encode(data):
    data = bytes(data)
    out, at = bytearray(), 0
    while at < len(data):
        run = 1
        while at + run < len(data) and run < 128 and data[at + run] == data[at]:
            run += 1
        if run >= 3:
            out.extend((0x80 | (run - 1), data[at]))
            at += run
        else:
            start = at
            at += run
            while at < len(data) and at - start < 128:
                following = 1
                while at + following < len(data) and following < 3 and data[at + following] == data[at]:
                    following += 1
                if following >= 3:
                    break
                at += min(following, 128 - (at - start))
            out.append(at - start - 1)
            out.extend(data[start:at])
    return bytes(out)


def packbits_decode(data, expected_size):
    if not 0 <= expected_size <= MAX_PIXELS:
        raise ValueError('PackBits decoded length outside bounds')
    out, at = bytearray(), 0
    while at < len(data):
        control = data[at]
        at += 1
        count = (control & 127) + 1
        if len(out) + count > expected_size:
            raise ValueError('PackBits output overrun')
        consumed = 1 if control & 128 else count
        if at + consumed > len(data):
            raise ValueError('truncated PackBits packet')
        out.extend(bytes([data[at]]) * count if control & 128 else data[at:at + count])
        at += consumed
    if len(out) != expected_size:
        raise ValueError('PackBits decoded length mismatch')
    return bytes(out)


@dataclass
class Prepared:
    rgb: Image.Image       # RGB PIL image, partial alpha already matted to OUTLINE
    mask: Image.Image      # L PIL image, exactly 0/255; same full frame size
    stats: dict


def prepare_pil(image, size, character=False, green_key=False):
    if not (0 < size[0] <= 1024 and 0 < size[1] <= 1024):
        raise ValueError('invalid display frame size')
    image = image.convert('RGBA')
    if green_key:
        pixels = list(image.get_flattened_data())
        image.putdata([(r, g, b, 0 if g > 120 and g > r * 1.5 and g > b * 1.5 else a)
                       for r, g, b, a in pixels])
    image = image.resize(size, Image.Resampling.LANCZOS)
    alpha = image.getchannel('A')
    # Reject alpha<16 interpolation/model noise, preserve genuine coverage otherwise.
    mask = alpha.point(lambda a: 255 if a >= 16 else 0)
    edge = ImageChops.subtract(mask, mask.filter(ImageFilter.MinFilter(3)))
    rgb = image.convert('RGB')
    colors, edges = list(rgb.get_flattened_data()), list(edge.get_flattened_data())
    canonical = 0
    if not character:
        for i, ((r, g, b), boundary) in enumerate(zip(colors, edges)):
            # Only near-target dark warm pigment on a one-pixel silhouette ring.
            # No character hair/face/garment receives this automatic rewrite.
            if boundary and max(abs(r - 59), abs(g - 39), abs(b - 48)) <= 32 and r > g and r >= b - 12:
                colors[i] = OUTLINE
                canonical += 1
        rgb.putdata(colors)
    rgb = Image.composite(rgb, Image.new('RGB', size, OUTLINE), alpha)
    return Prepared(rgb, mask, {'canonical_outline_pixels': canonical,
                                'character_canonicalization_disabled': bool(character),
                                'partial_alpha_pixels': sum(16 <= a < 255 for a in alpha.get_flattened_data()),
                                'alpha_threshold': 16})


def prepare_image(path, size, character=False, green_key=False):
    with Image.open(path) as image:
        return prepare_pil(image, size, character, green_key)


def build_palette(prepared):
    opaque = bytearray()
    for image in prepared:
        for rgb, mask in zip(image.rgb.get_flattened_data(), image.mask.get_flattened_data()):
            if mask:
                opaque.extend(rgb)
    if not opaque:
        return [0, rgb565(OUTLINE)]
    sample = Image.frombytes('RGB', (len(opaque) // 3, 1), bytes(opaque))
    quantized = sample.quantize(colors=62, method=Image.Quantize.MEDIANCUT,
                               dither=Image.Dither.NONE)
    palette = quantized.getpalette()
    words = [0, rgb565(OUTLINE)]
    # Frequency followed by RGB provides deterministic tie-breaking.
    for count, index in sorted(quantized.getcolors(), key=lambda x: (-x[0], tuple(palette[x[1]*3:x[1]*3+3]))):
        word = rgb565(palette[index * 3:index * 3 + 3])
        if word not in words[1:]:
            words.append(word)
    return words


def index_image(prepared, palette):
    if not 2 <= len(palette) <= 64 or palette[0] != 0 or palette[1] != rgb565(OUTLINE):
        raise ValueError('invalid shared palette')
    lookup = [expand565(palette[1])] + [expand565(p) for p in palette[1:]]
    lookup.extend([lookup[0]] * (256 - len(lookup)))
    pal = Image.new('P', (1, 1))
    pal.putpalette([v for color in lookup for v in color])
    q = prepared.rgb.quantize(palette=pal, dither=Image.Dither.NONE).tobytes()
    data = bytearray()
    for value, color, mask in zip(q, prepared.rgb.get_flattened_data(), prepared.mask.get_flattened_data()):
        data.append(0 if not mask else 1 if value == 0 or value >= len(palette) or color == OUTLINE else value)
    return bytes(data)


@dataclass
class SpritePack:
    groups: dict
    assets: dict

    def image(self, ident):
        asset = self.assets[ident]
        palette = self.groups[asset['group']]
        rgba = [(0, 0, 0, 0) if i == 0 else (*expand565(palette[i]), 255)
                for i in asset['indices']]
        image = Image.new('RGBA', asset['frame'])
        image.putdata(rgba)
        return image


def pack_sprites(entries):
    entries = sorted(entries, key=lambda e: e['id'])
    ids = [e['id'] for e in entries]
    if len(ids) != len(set(ids)) or len({fnv1a(i) for i in ids}) != len(ids):
        raise ValueError('duplicate sprite ID/hash collision')
    grouped = defaultdict(list)
    for entry in entries:
        grouped[entry['group']].append(entry['prepared'])
    names = sorted(grouped)
    palettes = {name: build_palette(grouped[name]) for name in names}
    strings, offsets = bytearray(), {}
    for name in names + ids:
        if not SAFE_NAME.fullmatch(name):
            raise ValueError('invalid sprite/group name')
        if name not in offsets:
            offsets[name] = len(strings)
            strings.extend(name.encode('utf-8') + b'\0')
    group_offset = HEADER.size
    index_offset = group_offset + len(names) * GROUP.size
    palette_offset = index_offset + len(entries) * ENTRY.size
    palette_bytes = b''.join(struct.pack('<' + 'H' * len(palettes[n]), *palettes[n]) for n in names)
    strings_offset = palette_offset + len(palette_bytes)
    data_offset = strings_offset + len(strings)
    groups, position = bytearray(), palette_offset
    for name in names:
        count = len(palettes[name])
        groups.extend(GROUP.pack(offsets[name], count, 0, position, count * 2, b'\0' * 16))
        position += count * 2
    directory, payload = bytearray(), bytearray()
    for entry in entries:
        image = entry['prepared']
        full = Image.frombytes('L', image.rgb.size, index_image(image, palettes[entry['group']]))
        bbox = full.getbbox() or (0, 0, 0, 0)
        x, y, right, bottom = bbox
        raw = full.crop(bbox).tobytes() if right > x else b''
        packed = packbits_encode(raw)
        flags = int(len(packed) < len(raw))
        encoded = packed if flags else raw
        directory.extend(ENTRY.pack(fnv1a(entry['id']), offsets[entry['id']], names.index(entry['group']),
                                     flags, *image.rgb.size, x, y, right-x, bottom-y,
                                     data_offset + len(payload), len(encoded), binascii.crc32(raw)))
        payload.extend(encoded)
    body = bytes(groups + directory) + palette_bytes + bytes(strings + payload)
    return HEADER.pack(b'KSP1', 1, HEADER.size, HEADER.size + len(body), len(names), len(entries),
                       group_offset, index_offset, strings_offset, data_offset, binascii.crc32(body)) + body


def unpack_sprites(data):
    data = bytes(data)
    if len(data) < HEADER.size:
        raise ValueError('truncated KSP1 header')
    magic, version, header, size, gc, count, go, io, so, po, crc = HEADER.unpack_from(data)
    if magic != b'KSP1' or version != 1 or header != HEADER.size or size != len(data):
        raise ValueError('invalid KSP1 header/version/size')
    if gc > 4096 or count > 10000 or go != header or io != go + gc*GROUP.size:
        raise ValueError('invalid KSP1 directory bounds')
    palette_start = io + count * ENTRY.size
    if not palette_start <= so <= po <= size or binascii.crc32(data[header:]) != crc:
        raise ValueError('KSP1 section bounds/CRC mismatch')

    def name(offset):
        start = so + offset
        end = data.find(b'\0', start, po) if so <= start < po else -1
        if end < 0:
            raise ValueError('KSP1 string offset/termination invalid')
        try:
            label = data[start:end].decode('utf-8')
            if not SAFE_NAME.fullmatch(label):
                raise ValueError('KSP1 unsafe/invalid ID')
            return label
        except UnicodeError as error:
            raise ValueError('KSP1 invalid UTF-8 ID') from error

    groups, group_names, expected_palette = {}, [], palette_start
    for i in range(gc):
        no, n, flags, offset, length, reserved = GROUP.unpack_from(data, go + i*GROUP.size)
        label = name(no)
        if not label or label in groups or not 2 <= n <= 64 or flags or reserved != b'\0'*16:
            raise ValueError('invalid KSP1 group')
        if offset != expected_palette or length != n*2 or offset + length > so:
            raise ValueError('invalid KSP1 palette bounds')
        palette = list(struct.unpack_from('<' + 'H'*n, data, offset))
        if palette[0] != 0 or palette[1] != rgb565(OUTLINE):
            raise ValueError('invalid KSP1 reserved colors')
        groups[label] = palette
        group_names.append(label)
        expected_palette += length
    if expected_palette != so:
        raise ValueError('KSP1 unclaimed palette bytes')
    assets, hashes, expected_payload = {}, set(), po
    for i in range(count):
        values = ENTRY.unpack_from(data, io + i*ENTRY.size)
        hashed, no, group, flags, fw, fh, x, y, w, h, offset, length, checksum = values
        ident = name(no)
        if not ident or ident in assets or hashed != fnv1a(ident) or hashed in hashes:
            raise ValueError('invalid/duplicate KSP1 ID/hash')
        if group >= gc or flags not in (0, 1) or not 0 < fw*fh <= MAX_PIXELS or fw > 1024 or fh > 1024:
            raise ValueError('invalid KSP1 frame/group/codec')
        if x+w > fw or y+h > fh or bool(w) != bool(h) or offset != expected_payload or offset+length > size:
            raise ValueError('invalid KSP1 crop/payload bounds')
        raw = packbits_decode(data[offset:offset+length], w*h) if flags else data[offset:offset+length]
        if len(raw) != w*h or binascii.crc32(raw) != checksum or any(p >= len(groups[group_names[group]]) for p in raw):
            raise ValueError('KSP1 pixels length/CRC/palette index invalid')
        frame = Image.new('L', (fw, fh))
        if w:
            frame.paste(Image.frombytes('L', (w, h), raw), (x, y))
        assets[ident] = {'group': group_names[group], 'frame': (fw, fh), 'crop': (x, y, w, h),
                         'indices': frame.tobytes(), 'encoded_bytes': length, 'offset': offset}
        hashes.add(hashed)
        expected_payload += length
    if expected_payload != size:
        raise ValueError('KSP1 trailing/unclaimed payload')
    return SpritePack(groups, assets)
