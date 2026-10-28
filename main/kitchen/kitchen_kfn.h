#ifndef KITCHEN_KFN_H
#define KITCHEN_KFN_H
#include <stddef.h>
#include <stdint.h>
#define KFN_MAX_GLYPH_BYTES 1024
typedef struct {
    const uint8_t* data;
    size_t size;
    uint32_t font_count, glyph_count, glyph_offset, payload_offset;
} kfn_pack;
typedef struct {
    uint32_t start, count;
    uint16_t px, bpp;
    int16_t ascent, descent;
    uint16_t line_height;
} kfn_font;
typedef struct {
    uint8_t width, height, advance;
    int8_t x, y;
    size_t length;
} kfn_glyph;
/* The validated input must remain unchanged for the lifetime of the pack. */
int kfn_open(kfn_pack* out, const uint8_t* data, size_t size);
int kfn_find_font(const kfn_pack* pack, const char* id, kfn_font* out);
/* Returns 1 for a valid glyph, 0 when missing or corrupt. */
int kfn_read_glyph(const kfn_pack* pack, const kfn_font* font, uint32_t codepoint, kfn_glyph* glyph,
                   uint8_t* coverage, size_t capacity);
#endif
