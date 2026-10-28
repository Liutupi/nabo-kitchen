#include "kitchen_krc.h"
#include <string.h>
static uint16_t u16(const uint8_t* p) { return (uint16_t)(p[0] | (uint16_t)p[1] << 8); }
static uint32_t u32(const uint8_t* p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static uint32_t u24(const uint8_t* p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16;
}
static uint32_t crc32(const uint8_t* p, size_t n) {
    uint32_t c = UINT32_MAX;
    for (size_t i = 0; i < n; ++i) {
        c ^= p[i];
        for (unsigned j = 0; j < 8; ++j)
            c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1u)));
    }
    return ~c;
}
int krc_open(krc_pack* out, const uint8_t* d, size_t size) {
    if (!out || !d || size < 48 || size > UINT32_MAX || memcmp(d, "KRC1", 4) || u16(d + 4) != 1 ||
        u16(d + 6) != 48 || u32(d + 8) != size || u32(d + 40) != 0x408 ||
        crc32(d + 48, size - 48) != u32(d + 44))
        return 0;
    uint32_t count = u32(d + 12), index = u32(d + 16), dict = u32(d + 20), dl = u32(d + 24),
             bitmap = u32(d + 28), width = u32(d + 32), body = u32(d + 36);
    if (!count || count > 65535 || index != 48 || (uint64_t)index + (uint64_t)count * 16 != dict ||
        (uint64_t)dict + dl != bitmap || (uint64_t)bitmap + (uint64_t)count * width != body ||
        body > size)
        return 0;
    size_t position = body;
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t* record = d + index + (size_t)i * 16;
        if (u24(record + 9) != (uint64_t)i * width ||
            (uint64_t)body + u32(record + 12) != position || position > size || size - position < 8)
            return 0;
        uint32_t compressed = u16(d + position), raw = u16(d + position + 2);
        if (!raw || raw > KRC_MAX_BODY || compressed > size - position - 8)
            return 0;
        position += 8 + compressed;
    }
    if (position != size)
        return 0;
    *out = (krc_pack){d, size, count, dict, dl, bitmap, width, body};
    return 1;
}
int krc_index_at(const krc_pack* p, uint32_t i, krc_index* out) {
    if (!p || !out || !p->data || i >= p->count)
        return 0;
    const uint8_t* r = p->data + 48 + (size_t)i * 16;
    *out =
        (krc_index){u32(r), u16(r + 5), r[4], r[7], r[8], p->data + p->bitmap_offset + u24(r + 9)};
    return 1;
}
static int bits(const uint8_t* data, size_t length, size_t* pos, unsigned count, unsigned* result) {
    if (*pos > length * 8 || count > length * 8 - *pos)
        return 0;
    unsigned value = 0;
    for (unsigned i = 0; i < count; ++i, ++*pos)
        value = (value << 1) | ((data[*pos / 8] >> (7 - *pos % 8)) & 1);
    *result = value;
    return 1;
}
int krc_read_body(const krc_pack* p, uint32_t i, uint8_t* out, size_t capacity) {
    if (!p || !p->data || !out || i >= p->count)
        return -1;
    const uint8_t* record = p->data + 48 + (size_t)i * 16;
    size_t at = (size_t)p->body_offset + u32(record + 12);
    if (at > p->size || p->size - at < 8)
        return -1;
    size_t clen = u16(p->data + at), length = u16(p->data + at + 2);
    if (!length || length > KRC_MAX_BODY || length > capacity || clen > p->size - at - 8)
        return -1;
    const uint8_t* input = p->data + at + 8;
    size_t bit = 0, written = 0;
    while (written < length) {
        unsigned tag, value, n;
        if (!bits(input, clen, &bit, 1, &tag))
            return -1;
        if (tag) {
            if (!bits(input, clen, &bit, 8, &value))
                return -1;
            out[written++] = (uint8_t)value;
        } else {
            if (!bits(input, clen, &bit, 8, &value) || !bits(input, clen, &bit, 4, &n))
                return -1;
            size_t distance = (size_t)value + 1, count = (size_t)n + 1;
            if (count > length - written)
                return -1;
            while (count--) {
                out[written] = distance <= written ? out[written - distance] : 0;
                ++written;
            }
        }
    }
    size_t remaining = clen * 8 - bit;
    unsigned padding;
    if (remaining > 7 ||
        (remaining && (!bits(input, clen, &bit, (unsigned)remaining, &padding) || padding)) ||
        crc32(out, length) != u32(p->data + at + 4))
        return -1;
    return (int)length;
}
