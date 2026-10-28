"""KAN1: bounded indexed RGB565 animation key/delta/repeat frames.

The codec is lossless with respect to prepared, palette-indexed frames. Source
video keying, temporal selection and palette reduction are separate operations.
"""
import binascii
from dataclasses import dataclass
import re
import struct

import _bootstrap  # noqa: F401
from PIL import Image

from asset_codec import OUTLINE, expand565, packbits_decode, packbits_encode, rgb565

HEADER = struct.Struct('<4sHH11I')
ACTION = struct.Struct('<16sHH4I')
FRAME = struct.Struct('<BB4H2I')
MAX_PIXELS = 1024 * 1024
MAX_ACTIONS = 256
MAX_FRAMES = 4096
MAX_TOTAL_DECODED_PIXELS = 64 * 1024 * 1024
KEY_RLE, DELTA_RLE, REPEAT, KEY_RAW, DELTA_RAW = range(5)
ID = re.compile(r'^[a-z][a-z0-9_]{0,14}$')


def _validate_palette(palette):
    if not 2 <= len(palette) <= 64 or palette[0] != 0 or palette[1] != rgb565(OUTLINE):
        raise ValueError('KAN1 palette requires transparent0, outline1 and <=64 colors')
    if any(not isinstance(p, int) or not 0 <= p <= 65535 for p in palette):
        raise ValueError('KAN1 invalid RGB565 palette word')


def _crop(raw, width, rectangle):
    x, y, w, h = rectangle
    return b''.join(raw[(y + row) * width + x:(y + row) * width + x + w] for row in range(h))


def _changed_rectangle(previous, current, width, height):
    changed = [i for i, (before, after) in enumerate(zip(previous, current)) if before != after]
    if not changed:
        return None
    xs = [i % width for i in changed]
    ys = [i // width for i in changed]
    x, y = min(xs), min(ys)
    return x, y, max(xs) - x + 1, max(ys) - y + 1


@dataclass
class AnimationPack:
    size: tuple
    anchor: tuple
    fps: int
    palette: list
    actions: dict

    def image(self, ident, frame):
        raw = self.actions[ident]['frames'][frame]
        image = Image.new('RGBA', self.size)
        image.putdata([(0, 0, 0, 0) if value == 0 else (*expand565(self.palette[value]), 255)
                       for value in raw])
        return image


def pack_animations(actions, palette, size=(192, 192), anchor=(96, 191), fps=12):
    """Return deterministic KAN1 bytes; each action contains full indexed frames."""
    _validate_palette(palette)
    width, height = size
    if not 0 < width <= 1024 or not 0 < height <= 1024 or width * height > MAX_PIXELS:
        raise ValueError('KAN1 invalid frame dimensions')
    if not (0 <= anchor[0] < width and 0 <= anchor[1] < height and 1 <= fps <= 120):
        raise ValueError('KAN1 invalid anchor/fps')
    actions = sorted(actions, key=lambda action: action['id'])
    ids = [action['id'] for action in actions]
    if not 0 < len(actions) <= MAX_ACTIONS or len(set(ids)) != len(ids) or any(not ID.fullmatch(i) for i in ids):
        raise ValueError('KAN1 invalid/duplicate action IDs')
    if sum(len(action['frames']) for action in actions) * width * height > MAX_TOTAL_DECODED_PIXELS:
        raise ValueError('KAN1 total decoded pixels outside host bounds')
    streams = []
    for action in actions:
        frames = action['frames']
        if not 0 < len(frames) <= MAX_FRAMES:
            raise ValueError('KAN1 invalid frame count')
        stream, previous = bytearray(), None
        for frame in frames:
            frame = bytes(frame)
            if len(frame) != width * height or any(p >= len(palette) for p in frame):
                raise ValueError('KAN1 frame length or palette index invalid')
            checksum = binascii.crc32(frame)
            rectangle = (0, 0, width, height)
            mode, payload = KEY_RAW, frame
            compressed = packbits_encode(frame)
            if len(compressed) < len(payload):
                mode, payload = KEY_RLE, compressed
            if previous is not None:
                changed = _changed_rectangle(previous, frame, width, height)
                if changed is None:
                    mode, rectangle, payload = REPEAT, (0, 0, 0, 0), b''
                else:
                    delta = _crop(frame, width, changed)
                    packed_delta = packbits_encode(delta)
                    delta_mode, delta_payload = (DELTA_RLE, packed_delta) if len(packed_delta) < len(delta) else (DELTA_RAW, delta)
                    if len(delta_payload) < len(payload):
                        mode, rectangle, payload = delta_mode, changed, delta_payload
            stream.extend(FRAME.pack(mode, 0, *rectangle, len(payload), checksum))
            stream.extend(payload)
            previous = frame
        streams.append(bytes(stream))
    palette_bytes = struct.pack('<' + 'H' * len(palette), *palette)
    index_offset = HEADER.size + len(palette_bytes)
    data_offset = index_offset + ACTION.size * len(actions)
    directory, data = bytearray(), bytearray()
    for action, stream in zip(actions, streams):
        label = action['id'].encode('ascii').ljust(16, b'\0')
        directory.extend(ACTION.pack(label, len(action['frames']), int(bool(action.get('loop'))),
                                     data_offset + len(data), len(stream), binascii.crc32(stream), 0))
        data.extend(stream)
    body = palette_bytes + bytes(directory) + bytes(data)
    return HEADER.pack(b'KAN1', 1, HEADER.size, HEADER.size + len(body), width, height,
                       *anchor, fps, len(palette), len(actions), index_offset, data_offset,
                       binascii.crc32(body)) + body


def unpack_animations(data):
    """Decode KAN1 with all lengths, offsets, rectangles, indices and CRCs bounded."""
    data = bytes(data)
    if len(data) < HEADER.size:
        raise ValueError('truncated KAN1 header')
    magic, version, header_size, total, width, height, ax, ay, fps, pc, count, io, po, crc = HEADER.unpack_from(data)
    if magic != b'KAN1' or version != 1 or header_size != HEADER.size or total != len(data):
        raise ValueError('invalid KAN1 header/version/size')
    if not 0 < width <= 1024 or not 0 < height <= 1024 or width * height > MAX_PIXELS:
        raise ValueError('invalid KAN1 dimensions')
    if not (0 <= ax < width and 0 <= ay < height and 1 <= fps <= 120):
        raise ValueError('invalid KAN1 anchor/fps')
    if not 2 <= pc <= 64 or not 0 < count <= MAX_ACTIONS:
        raise ValueError('invalid KAN1 palette/action count')
    if io != HEADER.size + pc * 2 or po != io + count * ACTION.size or po > total:
        raise ValueError('invalid KAN1 section bounds')
    if binascii.crc32(data[HEADER.size:]) != crc:
        raise ValueError('KAN1 body CRC mismatch')
    palette = list(struct.unpack_from('<' + 'H' * pc, data, HEADER.size))
    _validate_palette(palette)
    actions, expected, decoded_pixels = {}, po, 0
    for action_number in range(count):
        name_bytes, frame_count, flags, offset, length, checksum, reserved = ACTION.unpack_from(data, io + action_number * ACTION.size)
        zero = name_bytes.find(b'\0')
        if zero < 1 or name_bytes[zero:] != b'\0' * (16 - zero):
            raise ValueError('invalid KAN1 action name termination')
        try:
            ident = name_bytes[:zero].decode('ascii')
        except UnicodeError as error:
            raise ValueError('invalid KAN1 action ID') from error
        if not ID.fullmatch(ident) or ident in actions or not 0 < frame_count <= MAX_FRAMES or flags not in (0, 1) or reserved:
            raise ValueError('invalid KAN1 action metadata')
        if offset != expected or offset + length > total or length < frame_count * FRAME.size:
            raise ValueError('invalid KAN1 action bounds')
        decoded_pixels += frame_count * width * height
        if decoded_pixels > MAX_TOTAL_DECODED_PIXELS:
            raise ValueError('KAN1 total decoded pixels outside host bounds')
        end = offset + length
        if binascii.crc32(data[offset:end]) != checksum:
            raise ValueError('KAN1 action CRC mismatch')
        position, previous, frames, records = offset, None, [], []
        for frame_number in range(frame_count):
            if position + FRAME.size > end:
                raise ValueError('truncated KAN1 frame header')
            mode, reserved, x, y, w, h, payload_size, full_crc = FRAME.unpack_from(data, position)
            position += FRAME.size
            if reserved or mode not in range(5) or position + payload_size > end:
                raise ValueError('invalid KAN1 frame metadata/bounds')
            payload = data[position:position + payload_size]
            position += payload_size
            if mode == REPEAT:
                if previous is None or (x, y, w, h, payload_size) != (0, 0, 0, 0, 0):
                    raise ValueError('invalid KAN1 repeat frame')
                frame = previous
            else:
                if not w or not h or x + w > width or y + h > height:
                    raise ValueError('KAN1 rectangle outside frame')
                if mode in (KEY_RLE, KEY_RAW) and (x, y, w, h) != (0, 0, width, height):
                    raise ValueError('KAN1 key frame is not full size')
                if mode in (DELTA_RLE, DELTA_RAW) and previous is None:
                    raise ValueError('KAN1 delta has no previous frame')
                raw = packbits_decode(payload, w * h) if mode in (KEY_RLE, DELTA_RLE) else payload
                if len(raw) != w * h or any(p >= pc for p in raw):
                    raise ValueError('KAN1 decoded rectangle length/palette index invalid')
                if mode in (KEY_RLE, KEY_RAW):
                    frame = raw
                else:
                    restored = bytearray(previous)
                    for row in range(h):
                        at = (y + row) * width + x
                        restored[at:at + w] = raw[row * w:(row + 1) * w]
                    frame = bytes(restored)
            if binascii.crc32(frame) != full_crc:
                raise ValueError('KAN1 reconstructed frame CRC mismatch')
            frames.append(frame)
            records.append({'mode': mode, 'rectangle': (x, y, w, h), 'payload_bytes': payload_size})
            previous = frame
        if position != end:
            raise ValueError('KAN1 unclaimed action bytes')
        actions[ident] = {'loop': bool(flags), 'frames': frames, 'records': records, 'encoded_bytes': length}
        expected = end
    if expected != total:
        raise ValueError('KAN1 trailing/unclaimed payload')
    return AnimationPack((width, height), (ax, ay), fps, palette, actions)
