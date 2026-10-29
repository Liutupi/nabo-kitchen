#include "kitchen_kan.h"
#include <string.h>
static uint16_t u16(const uint8_t* p) { return (uint16_t)(p[0] | (uint16_t)p[1] << 8); }
static uint32_t u32(const uint8_t* p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
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
static int name_valid(const uint8_t* p) {
    if (p[0] < 'a' || p[0] > 'z')
        return 0;
    unsigned i = 1;
    for (; i < 16 && p[i]; ++i)
        if (!((p[i] >= 'a' && p[i] <= 'z') || (p[i] >= '0' && p[i] <= '9') || p[i] == '_'))
            return 0;
    if (i == 16)
        return 0;
    for (; i < 16; ++i)
        if (p[i])
            return 0;
    return 1;
}
int kan_open(kan_pack* out, const uint8_t* d, size_t size) {
    if (!out || !d || size < 52 || size > UINT32_MAX || memcmp(d, "KAN1", 4) || u16(d + 4) != 1 ||
        u16(d + 6) != 52 || u32(d + 8) != size || crc32(d + 52, size - 52) != u32(d + 48))
        return 0;
    uint32_t w = u32(d + 12), h = u32(d + 16), ax = u32(d + 20), ay = u32(d + 24),
             fps = u32(d + 28), pal = u32(d + 32), count = u32(d + 36), io = u32(d + 40),
             data = u32(d + 44);
    if (!w || w > 1024 || !h || h > 1024 || ax >= w || ay >= h || !fps || fps > 120 || pal < 2 ||
        pal > 64 || !count || count > 256 || io != 52 + pal * 2 ||
        (uint64_t)io + (uint64_t)count * 36 != data || data > size || u16(d + 52) != 0 ||
        u16(d + 54) != 0x3926)
        return 0;
    size_t position = data;
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t* a = d + io + (size_t)i * 36;
        unsigned frames = u16(a + 16), flags = u16(a + 18);
        uint32_t offset = u32(a + 20), length = u32(a + 24);
        if (!name_valid(a) || !frames || frames > 4096 || flags > 1 || u32(a + 32) ||
            offset != position || offset > size || length > size - offset ||
            crc32(d + offset, length) != u32(a + 28))
            return 0;
        size_t at = offset, end = offset + length;
        for (unsigned j = 0; j < frames; ++j) {
            if (at > end || end - at < 18)
                return 0;
            const uint8_t* f = d + at;
            unsigned mode = f[0], x = u16(f + 2), y = u16(f + 4), fw = u16(f + 6), fh = u16(f + 8);
            uint32_t bytes = u32(f + 10);
            at += 18;
            if (f[1] || mode > 4 || bytes > end - at || (!j && mode != 0 && mode != 3))
                return 0;
            if (mode == 2) {
                if (x || y || fw || fh || bytes)
                    return 0;
            } else {
                if (!fw || !fh || x + fw > w || y + fh > h)
                    return 0;
                if ((mode == 0 || mode == 3) && (x || y || fw != w || fh != h))
                    return 0;
                if ((mode == 3 || mode == 4) && (uint64_t)fw * fh != bytes)
                    return 0;
            }
            at += bytes;
        }
        if (at != end)
            return 0;
        position = end;
    }
    if (position != size)
        return 0;
    *out = (kan_pack){d, size, w, h, fps, pal, count, io};
    return 1;
}
int kan_find(const kan_pack* p, const char* id, kan_action* out) {
    if (!p || !p->data || !id || !out)
        return 0;
    size_t n = strlen(id);
    if (!n || n > 15)
        return 0;
    for (uint32_t i = 0; i < p->action_count; ++i) {
        const uint8_t* a = p->data + p->index_offset + (size_t)i * 36;
        if (!memcmp(a, id, n) && !a[n]) {
            *out = (kan_action){u16(a + 16), u16(a + 18), u32(a + 20), u32(a + 24)};
            return 1;
        }
    }
    return 0;
}
int kan_start(kan_player* p, const kan_pack* pack, const kan_action* a, uint8_t* frame,
              size_t capacity) {
    if (!p || !pack || !pack->data || !a || !frame || !a->frames || a->offset > pack->size ||
        a->length > pack->size - a->offset || (uint64_t)pack->width * pack->height > capacity)
        return 0;
    *p = (kan_player){pack, *a, frame, capacity, a->offset, 0, 0};
    return 1;
}
int kan_next(kan_player* p) {
    if (!p || !p->pack || p->invalid)
        return -1;
    if (p->next == p->action.frames)
        return 0;
    const kan_pack* pack = p->pack;
    size_t end = (size_t)p->action.offset + p->action.length, at = p->position;
    if (at > end || end - at < 18) {
        p->invalid = 1;
        return -1;
    }
    const uint8_t* f = pack->data + at;
    unsigned mode = f[0], x = u16(f + 2), y = u16(f + 4), w = u16(f + 6), h = u16(f + 8);
    size_t bytes = u32(f + 10);
    at += 18;
    if (mode > 4 || bytes > end - at || x + w > pack->width || y + h > pack->height) {
        p->invalid = 1;
        return -1;
    }
    size_t n = (size_t)w * h, in = 0, written = 0, run = 0;
    int repeat = 0;
    uint8_t value = 0;
    while (written < n) {
        if (mode == 3 || mode == 4) {
            if (in >= bytes)
                break;
            value = pack->data[at + in++];
        } else {
            if (!run) {
                if (in >= bytes)
                    break;
                uint8_t ctl = pack->data[at + in++];
                run = (ctl & 127) + 1;
                repeat = !!(ctl & 128);
                if (repeat) {
                    if (in >= bytes)
                        break;
                    value = pack->data[at + in++];
                }
            }
            if (!repeat) {
                if (in >= bytes)
                    break;
                value = pack->data[at + in++];
            }
            --run;
        }
        if (value >= pack->palette_count)
            break;
        p->frame[(y + written / w) * pack->width + x + written % w] = value;
        ++written;
    }
    if (written != n || in != bytes || run ||
        crc32(p->frame, (size_t)pack->width * pack->height) != u32(f + 14)) {
        p->invalid = 1;
        return -1;
    }
    p->position = at + bytes;
    ++p->next;
    return 1;
}
