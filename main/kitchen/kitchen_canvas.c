#include "kitchen_canvas.h"
#include <string.h>
static void pixel(kg_canvas* c, int x, int y, uint16_t color) {
    if (x >= c->clip_x0 && x < c->clip_x1 && y >= c->clip_y0 && y < c->clip_y1)
        c->pixels[(y - c->y) * c->width + x] = color;
}
void kg_init(kg_canvas* c, uint16_t* pixels, int width, int y, int height, uint16_t bg) {
    *c = (kg_canvas){pixels, width, y, height, 0, 0, y, width, y + height};
    for (int i = 0; i < width * height; ++i)
        pixels[i] = bg;
}
void kg_clip(kg_canvas* c, int x, int y, int w, int h) {
    if (!c)
        return;
    int64_t x1 = (int64_t)x + w, y1 = (int64_t)y + h;
    if (x > c->clip_x0)
        c->clip_x0 = x;
    if (y > c->clip_y0)
        c->clip_y0 = y;
    if (x1 < c->clip_x1)
        c->clip_x1 = (int)x1;
    if (y1 < c->clip_y1)
        c->clip_y1 = (int)y1;
    if (c->clip_x1 < c->clip_x0)
        c->clip_x1 = c->clip_x0;
    if (c->clip_y1 < c->clip_y0)
        c->clip_y1 = c->clip_y0;
}
void kg_rect(kg_canvas* c, int x, int y, int w, int h, uint16_t color) {
    if (w <= 0 || h <= 0)
        return;
    int x0 = x < 0 ? 0 : x, y0 = y < c->y ? c->y : y, x1 = x + w > c->width ? c->width : x + w,
        y1 = y + h > c->y + c->height ? c->y + c->height : y + h;
    for (int yy = y0; yy < y1; ++yy)
        for (int xx = x0; xx < x1; ++xx)
            pixel(c, xx, yy, color);
}
void kg_round_rect(kg_canvas* c, int x, int y, int w, int h, unsigned radius, uint16_t color) {
    if (!c || w <= 0 || h <= 0)
        return;
    unsigned r = radius;
    if (r > (unsigned)w / 2)
        r = (unsigned)w / 2;
    if (r > (unsigned)h / 2)
        r = (unsigned)h / 2;
    if (!r) {
        kg_rect(c, x, y, w, h, color);
        return;
    }
    for (int row = 0; row < h; ++row) {
        unsigned inset = 0;
        int dy = row < (int)r ? (int)r - row - 1 : row >= h - (int)r ? row - (h - (int)r) : 0;
        if (dy)
            while (inset < r) {
                int dx = (int)r - (int)inset - 1;
                if (dx * dx + dy * dy <= (int)(r * r))
                    break;
                ++inset;
            }
        kg_rect(c, x + (int)inset, y + row, w - (int)inset * 2, 1, color);
    }
}
void kg_circle(kg_canvas* c, int x, int y, unsigned r, uint16_t color) {
    if (!c || r > 1024)
        return;
    int y0 = c->clip_y0 - y > -(int)r ? c->clip_y0 - y : -(int)r,
        y1 = c->clip_y1 - y < (int)r + 1 ? c->clip_y1 - y : (int)r + 1;
    int x0 = c->clip_x0 - x > -(int)r ? c->clip_x0 - x : -(int)r,
        x1 = c->clip_x1 - x < (int)r + 1 ? c->clip_x1 - x : (int)r + 1;
    for (int dy = y0; dy < y1; ++dy)
        for (int dx = x0; dx < x1; ++dx)
            if (dx * dx + dy * dy <= (int)(r * r))
                pixel(c, x + dx, y + dy, color);
}
static uint16_t blend(uint16_t bg, uint16_t fg, unsigned a, unsigned max) {
    unsigned r = (((bg >> 11) & 31) * (max - a) + ((fg >> 11) & 31) * a + max / 2) / max;
    unsigned g = (((bg >> 5) & 63) * (max - a) + ((fg >> 5) & 63) * a + max / 2) / max;
    unsigned b = ((bg & 31) * (max - a) + (fg & 31) * a + max / 2) / max;
    return (uint16_t)((r << 11) | (g << 5) | b);
}
static int decode_utf8(const char** ptr, uint32_t* cp) {
    const unsigned char* p = (const unsigned char*)*ptr;
    uint32_t x = *p++;
    unsigned n;
    if (x < 0x80)
        n = 0;
    else if (x >= 0xc2 && x <= 0xdf) {
        x &= 31;
        n = 1;
    } else if (x >= 0xe0 && x <= 0xef) {
        x &= 15;
        n = 2;
    } else if (x >= 0xf0 && x <= 0xf4) {
        x &= 7;
        n = 3;
    } else
        return 0;
    unsigned total = n;
    while (n--) {
        if ((*p & 0xc0) != 0x80)
            return 0;
        x = (x << 6) | (*p++ & 63);
    }
    if ((total == 2 && x < 0x800) || (total == 3 && x < 0x10000) || x > 0x10ffff ||
        (x >= 0xd800 && x <= 0xdfff))
        return 0;
    *cp = x;
    *ptr = (const char*)p;
    return 1;
}
int kg_text(kg_canvas* c, const kfn_pack* pack, const kfn_font* font, int x, int top, int wrap,
            uint16_t color, const char* text) {
    return kg_text_line(c, pack, font, x, top, wrap, font ? font->line_height : 0, color, text);
}
int kg_text_line(kg_canvas* c, const kfn_pack* pack, const kfn_font* font, int x, int top, int wrap,
                 int line_height, uint16_t color, const char* text) {
    if (!c || !pack || !font || !text)
        return -1;
    if (line_height <= 0)
        return -1;
    int start = x, baseline = top + font->ascent;
    uint8_t bytes[KFN_MAX_GLYPH_BYTES];
    while (*text) {
        uint32_t cp;
        if (!decode_utf8(&text, &cp)) {
            c->error = 1;
            return -1;
        }
        if (cp == '\n') {
            x = start;
            baseline += line_height;
            continue;
        }
        kfn_glyph glyph;
        if (!kfn_read_glyph(pack, font, cp, &glyph, bytes, sizeof(bytes))) {
            c->error = 1;
            return -1;
        }
        if (wrap > 0 && x > start && x + glyph.advance > start + wrap) {
            x = start;
            baseline += line_height;
        }
        unsigned mask = (1u << font->bpp) - 1;
        for (unsigned row = 0; row < glyph.height; ++row) {
            int py = baseline + glyph.y + (int)row;
            if (py < c->clip_y0 || py >= c->clip_y1)
                continue;
            for (unsigned col = 0; col < glyph.width; ++col) {
                int px = x + glyph.x + (int)col;
                if (px < c->clip_x0 || px >= c->clip_x1)
                    continue;
                size_t bit = ((size_t)row * glyph.width + col) * font->bpp;
                unsigned a = (bytes[bit / 8] >> (8 - font->bpp - bit % 8)) & mask;
                if (a) {
                    uint16_t* dest = &c->pixels[(py - c->y) * c->width + px];
                    *dest = blend(*dest, color, a, mask);
                }
            }
        }
        x += glyph.advance;
    }
    return baseline + font->descent;
}
void kg_indexed(kg_canvas* c, int x, int y, unsigned w, unsigned h, const uint8_t* indices,
                const uint8_t* palette, unsigned colors) {
    if (!c || !indices || !palette || !colors || colors > 64) {
        if (c)
            c->error = 1;
        return;
    }
    for (unsigned row = 0; row < h; ++row) {
        int py = y + (int)row;
        if (py < c->y || py - c->y >= c->height)
            continue;
        for (unsigned col = 0; col < w; ++col) {
            unsigned i = indices[(size_t)row * w + col];
            if (i >= colors) {
                c->error = 1;
                return;
            }
            if (i)
                pixel(c, x + (int)col, py,
                      (uint16_t)(palette[i * 2] | (uint16_t)palette[i * 2 + 1] << 8));
        }
    }
}
void kg_indexed_scaled(kg_canvas* c, int x, int y, unsigned w, unsigned h, const uint8_t* indices,
                       unsigned sw, unsigned sh, const uint8_t* palette, unsigned colors) {
    if (!c || !indices || !palette || !w || !h || !sw || !sh || w > 1024 || h > 1024 || sw > 1024 ||
        sh > 1024 || !colors || colors > 64) {
        if (c)
            c->error = 1;
        return;
    }
    for (unsigned row = 0; row < h; ++row) {
        int py = y + (int)row;
        if (py < c->y || py - c->y >= c->height)
            continue;
        for (unsigned col = 0; col < w; ++col) {
            unsigned i = indices[(size_t)(row * sh / h) * sw + col * sw / w];
            if (i >= colors) {
                c->error = 1;
                return;
            }
            if (i)
                pixel(c, x + (int)col, py,
                      (uint16_t)(palette[i * 2] | (uint16_t)palette[i * 2 + 1] << 8));
        }
    }
}
typedef struct {
    kg_canvas* canvas;
    const ksp_sprite* sprite;
    int x, y;
    unsigned width, height;
} sprite_context;
static int sprite_row(void* context, unsigned x, unsigned y, const uint8_t* indices,
                      unsigned width) {
    sprite_context* s = context;
    unsigned fw = s->sprite->frame_width, fh = s->sprite->frame_height;
    unsigned y0 = (y * s->height + fh - 1) / fh, y1 = ((y + 1) * s->height + fh - 1) / fh;
    unsigned x0 = (x * s->width + fw - 1) / fw, x1 = ((x + width) * s->width + fw - 1) / fw;
    for (unsigned dy = y0; dy < y1; ++dy) {
        int py = s->y + (int)dy;
        if (py < s->canvas->y || py - s->canvas->y >= s->canvas->height)
            continue;
        for (unsigned dx = x0; dx < x1; ++dx) {
            unsigned source = dx * fw / s->width - x, value = indices[source];
            if (value)
                pixel(s->canvas, s->x + (int)dx, py,
                      (uint16_t)(s->sprite->palette[value * 2] |
                                 (uint16_t)s->sprite->palette[value * 2 + 1] << 8));
        }
    }
    return !s->canvas->error;
}
int kg_sprite(kg_canvas* c, const ksp_pack* pack, const ksp_sprite* s, int x, int y) {
    return kg_sprite_scaled(c, pack, s, x, y, s ? s->frame_width : 0, s ? s->frame_height : 0);
}
int kg_sprite_scaled(kg_canvas* c, const ksp_pack* pack, const ksp_sprite* s, int x, int y,
                     unsigned width, unsigned height) {
    if (!c || !pack || !s)
        return 0;
    if (!width || !height || width > 1024 || height > 1024 || !s->frame_width || !s->frame_height)
        return 0;
    uint8_t row[1024];
    sprite_context ctx = {c, s, x, y, width, height};
    if (!ksp_read_rows(pack, s, row, sizeof(row), sprite_row, &ctx)) {
        c->error = 1;
        return 0;
    }
    return 1;
}
