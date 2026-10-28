"""Heatshrink 0.4 compatible bitstream, window=8/lookahead=4, bounded decoder.

The encoder is the pinned heatshrink2 C binding. The independent reader follows
atomicobject/heatshrink's literal/back-reference MSB-first stream.
"""
import _bootstrap  # noqa: F401
import heatshrink2

WINDOW_BITS = 8
LOOKAHEAD_BITS = 4


def encode(raw):
    return heatshrink2.compress(bytes(raw), window_sz2=WINDOW_BITS,
                               lookahead_sz2=LOOKAHEAD_BITS)


def decode(data, expected_size, limit=2048):
    if not 0 <= expected_size <= limit:
        raise ValueError('heatshrink decoded length exceeds limit')
    data = bytes(data)
    position, out = 0, bytearray()

    def bits(count):
        nonlocal position
        if position + count > len(data) * 8:
            raise ValueError('truncated heatshrink token')
        value = 0
        for _ in range(count):
            value = (value << 1) | ((data[position // 8] >> (7 - position % 8)) & 1)
            position += 1
        return value

    while len(out) < expected_size:
        if bits(1):
            out.append(bits(8))
        else:
            distance, count = bits(WINDOW_BITS) + 1, bits(LOOKAHEAD_BITS) + 1
            if len(out) + count > expected_size:
                raise ValueError('heatshrink back-reference overruns decoded length')
            for _ in range(count):
                out.append(out[-distance] if distance <= len(out) else 0)
    remaining = len(data) * 8 - position
    if remaining > 7 or (remaining and bits(remaining) != 0):
        raise ValueError('noncanonical heatshrink trailing data/padding')
    return bytes(out)
