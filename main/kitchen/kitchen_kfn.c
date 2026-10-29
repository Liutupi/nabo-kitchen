#include "kitchen_kfn.h"
#include <string.h>
#include "kitchen_crc32.h"
static uint16_t u16(const uint8_t* p) { return (uint16_t)(p[0] | (uint16_t)p[1] << 8); }
static uint32_t u32(const uint8_t* p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static uint32_t crc32(const uint8_t* p, size_t n) {
    return kcrc32(p, n);
}
int kfn_open(kfn_pack* out, const uint8_t* d, size_t size) {
    if (!out || !d || size < 36 || size > UINT32_MAX || memcmp(d, "KFN1", 4) || u16(d + 4) != 1 ||
        u16(d + 6) != 16)
        return 0;
    uint32_t fonts = u32(d + 8), glyphs = u32(d + 12), fo = u32(d + 16), go = u32(d + 20),
             po = u32(d + 24);
    if (!fonts || fonts > 32 || glyphs > 100000 || fo != 36 ||
        (uint64_t)fo + (uint64_t)fonts * 36 != go || (uint64_t)go + (uint64_t)glyphs * 16 != po ||
        po > size || u32(d + 28) != size || crc32(d + 36, size - 36) != u32(d + 32))
        return 0;
    uint32_t next = 0;
    for (uint32_t i = 0; i < fonts; ++i) {
        const uint8_t* f = d + fo + (size_t)i * 36;
        uint16_t px = u16(f + 16), bpp = u16(f + 18);
        uint32_t start = u32(f + 20), count = u32(f + 24);
        if (!f[0] || !memchr(f, 0, 16) || !px || px > 64 || (bpp != 1 && bpp != 2) ||
            start != next || (uint64_t)start + count > glyphs || u16(f + 34) ||
            u16(f + 28) > 32767 || u16(f + 30) > 32767 || u16(f + 32) < px || u16(f + 32) > 256)
            return 0;
        uint32_t previous = 0;
        for (uint32_t j = 0; j < count; ++j) {
            const uint8_t* g = d + go + (size_t)(start + j) * 16;
            uint32_t cp = u32(g), at = u32(g + 10);
            uint16_t len = u16(g + 14);
            if ((j && cp <= previous) || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff) ||
                g[4] > 64 || g[5] > 64 || g[8] > 128 || g[9] > 1 || at < po ||
                (uint64_t)at + len > size)
                return 0;
            previous = cp;
        }
        next += count;
    }
    if (next != glyphs)
        return 0;
    *out = (kfn_pack){d, size, fonts, glyphs, go, po};
    return 1;
}
int kfn_find_font(const kfn_pack* p, const char* id, kfn_font* out) {
    if (!p || !p->data || !id || !out)
        return 0;
    size_t len = strlen(id);
    if (!len || len > 15)
        return 0;
    for (uint32_t i = 0; i < p->font_count; ++i) {
        const uint8_t* f = p->data + 36 + (size_t)i * 36;
        if (!memcmp(f, id, len) && f[len] == 0) {
            *out = (kfn_font){u32(f + 20),          u32(f + 24),          u16(f + 16), u16(f + 18),
                              (int16_t)u16(f + 28), (int16_t)u16(f + 30), u16(f + 32)};
            return 1;
        }
    }
    return 0;
}
int kfn_read_glyph(const kfn_pack* p, const kfn_font* f, uint32_t cp, kfn_glyph* out,
                   uint8_t* buffer, size_t capacity) {
    if (!p || !p->data || !f || !out || !buffer || f->start > p->glyph_count ||
        f->count > p->glyph_count - f->start || (f->bpp != 1 && f->bpp != 2))
        return 0;
    uint32_t lo = 0, hi = f->count;
    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2;
        const uint8_t* g = p->data + p->glyph_offset + (size_t)(f->start + mid) * 16;
        if (u32(g) < cp)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo == f->count)
        return 0;
    const uint8_t* g = p->data + p->glyph_offset + (size_t)(f->start + lo) * 16;
    if (u32(g) != cp)
        return 0;
    size_t expected = ((size_t)g[4] * g[5] * f->bpp + 7) / 8, encoded = u16(g + 14),
           at = u32(g + 10), in = 0, written = 0;
    if (expected > capacity || expected > KFN_MAX_GLYPH_BYTES || at < p->payload_offset ||
        at > p->size || encoded > p->size - at)
        return 0;
    if (!g[9]) {
        if (encoded != expected)
            return 0;
        memcpy(buffer, p->data + at, expected);
        written = expected;
    } else
        while (in < encoded) {
            uint8_t control = p->data[at + in++];
            size_t n = (control & 127) + 1;
            if (n > expected - written)
                return 0;
            if (control & 128) {
                if (in >= encoded)
                    return 0;
                memset(buffer + written, p->data[at + in++], n);
            } else {
                if (n > encoded - in)
                    return 0;
                memcpy(buffer + written, p->data + at + in, n);
                in += n;
            }
            written += n;
        }
    unsigned used = ((unsigned)g[4] * g[5] * f->bpp) % 8;
    if (written != expected ||
        (used && expected && (buffer[expected - 1] & ((1u << (8 - used)) - 1))))
        return 0;
    *out = (kfn_glyph){g[4], g[5], g[8], (int8_t)g[6], (int8_t)g[7], expected};
    return 1;
}
