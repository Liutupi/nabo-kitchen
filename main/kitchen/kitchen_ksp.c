#include "kitchen_ksp.h"
#include <string.h>
#include "kitchen_crc32.h"
static uint16_t u16(const uint8_t* p) { return (uint16_t)(p[0] | (uint16_t)p[1] << 8); }
static uint32_t u32(const uint8_t* p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static uint32_t crc_step(uint32_t c, uint8_t x) {
    c ^= x;
    for (unsigned j = 0; j < 8; ++j)
        c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1u)));
    return c;
}
static uint32_t crc32(const uint8_t* p, size_t n) {
    return kcrc32(p, n);
}
static int valid_name(const uint8_t* d, size_t start, size_t end) {
    if (start >= end)
        return 0;
    size_t n = 0;
    while (start + n < end && d[start + n]) {
        unsigned c = d[start + n++];
        if (n > 128 || !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                         (c >= '0' && c <= '9') || c == '_' || c == '-'))
            return 0;
    }
    return n && start + n < end;
}
int ksp_open(ksp_pack* out, const uint8_t* d, size_t size) {
    if (!out || !d || size < 40 || size > UINT32_MAX || memcmp(d, "KSP1", 4) || u16(d + 4) != 1 ||
        u16(d + 6) != 40 || u32(d + 8) != size || crc32(d + 40, size - 40) != u32(d + 36))
        return 0;
    uint32_t gc = u32(d + 12), count = u32(d + 16), go = u32(d + 20), io = u32(d + 24),
             so = u32(d + 28), po = u32(d + 32);
    if (gc > 4096 || count > 10000 || go != 40 || (uint64_t)go + (uint64_t)gc * 32 != io ||
        (uint64_t)io + (uint64_t)count * 36 > so || so > po || po > size)
        return 0;
    size_t palette = (size_t)io + (size_t)count * 36;
    for (uint32_t i = 0; i < gc; ++i) {
        const uint8_t* g = d + go + (size_t)i * 32;
        uint32_t name = u32(g), at = u32(g + 8), len = u32(g + 12);
        unsigned n = u16(g + 4);
        if ((uint64_t)so + name >= po || !valid_name(d, so + name, po) || n < 2 || n > 64 ||
            u16(g + 6) || at != palette || len != n * 2 || (uint64_t)at + len > so ||
            u16(d + at) != 0 || u16(d + at + 2) != 0x3926)
            return 0;
        for (unsigned j = 16; j < 32; ++j)
            if (g[j])
                return 0;
        palette += len;
    }
    if (palette != so)
        return 0;
    size_t position = po;
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t* r = d + io + (size_t)i * 36;
        uint32_t name = u32(r + 4), at = u32(r + 24), len = u32(r + 28);
        unsigned group = u16(r + 8), codec = u16(r + 10), fw = u16(r + 12), fh = u16(r + 14),
                 x = u16(r + 16), y = u16(r + 18), w = u16(r + 20), h = u16(r + 22);
        if ((uint64_t)so + name >= po || !valid_name(d, so + name, po) || group >= gc ||
            codec > 1 || !fw || !fh || fw > 1024 || fh > 1024 || x + w > fw || y + h > fh ||
            !!w != !!h || at != position || at > size || len > size - at)
            return 0;
        position += len;
    }
    if (position != size)
        return 0;
    *out = (ksp_pack){d, size, gc, count, io, so, po};
    return 1;
}
int ksp_find(const ksp_pack* p, const char* id, ksp_sprite* out) {
    if (!p || !p->data || !id || !out)
        return 0;
    size_t length = strlen(id);
    if (!length || length > 128)
        return 0;
    for (uint32_t i = 0; i < p->count; ++i) {
        const uint8_t* r = p->data + p->index_offset + (size_t)i * 36;
        size_t name = p->strings_offset + u32(r + 4);
        if (name + length >= p->payload_offset || memcmp(p->data + name, id, length) ||
            p->data[name + length])
            continue;
        const uint8_t* g = p->data + 40 + (size_t)u16(r + 8) * 32;
        *out = (ksp_sprite){u16(r + 12),          u16(r + 14), u16(r + 16), u16(r + 18),
                            u16(r + 20),          u16(r + 22), u16(g + 4),  u16(r + 10),
                            p->data + u32(g + 8), u32(r + 24), u32(r + 28), u32(r + 32)};
        return 1;
    }
    return 0;
}
int ksp_read_rows(const ksp_pack* p, const ksp_sprite* s, uint8_t* row, size_t cap, ksp_row_fn send,
                  void* ctx) {
    if (!p || !p->data || !s || !row || !send || s->width > cap || s->offset > p->size ||
        s->length > p->size - s->offset || s->codec > 1 || !s->palette_count ||
        s->palette_count > 64)
        return 0;
    size_t at = 0, run = 0;
    int repeat = 0;
    uint8_t value = 0;
    uint32_t crc = UINT32_MAX;
    for (unsigned y = 0; y < s->height; ++y) {
        for (unsigned x = 0; x < s->width; ++x) {
            if (!s->codec) {
                if (at >= s->length)
                    return 0;
                value = p->data[s->offset + at++];
            } else {
                if (!run) {
                    if (at >= s->length)
                        return 0;
                    uint8_t ctl = p->data[s->offset + at++];
                    run = (ctl & 127) + 1;
                    repeat = !!(ctl & 128);
                    if (repeat) {
                        if (at >= s->length)
                            return 0;
                        value = p->data[s->offset + at++];
                    }
                }
                if (!repeat) {
                    if (at >= s->length)
                        return 0;
                    value = p->data[s->offset + at++];
                }
                --run;
            }
            if (value >= s->palette_count)
                return 0;
            row[x] = value;
            crc = crc_step(crc, value);
        }
        if (!send(ctx, s->x, s->y + y, row, s->width))
            return 0;
    }
    return at == s->length && !run && ~crc == s->crc;
}
