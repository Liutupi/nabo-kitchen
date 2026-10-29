#ifndef KITCHEN_CANVAS_H
#define KITCHEN_CANVAS_H
#include <stddef.h>
#include <stdint.h>
#include "kitchen_kfn.h"
#include "kitchen_ksp.h"
typedef struct {
    uint16_t* pixels;
    int width, y, height;
    int error;
    int clip_x0, clip_y0, clip_x1, clip_y1;
} kg_canvas;
void kg_init(kg_canvas* c, uint16_t* pixels, int width, int y, int height, uint16_t background);
void kg_clip(kg_canvas* c, int x, int y, int width, int height);
void kg_rect(kg_canvas* c, int x, int y, int w, int h, uint16_t color);
void kg_round_rect(kg_canvas* c, int x, int y, int w, int h, unsigned radius, uint16_t color);
void kg_circle(kg_canvas* c, int center_x, int center_y, unsigned radius, uint16_t color);
/* top is font line top; wrap=0 disables line wrapping. Returns bottom y or -1. */
int kg_text(kg_canvas* c, const kfn_pack* pack, const kfn_font* font, int x, int top, int wrap,
            uint16_t color, const char* utf8);
int kg_text_line(kg_canvas* c, const kfn_pack* pack, const kfn_font* font, int x, int top, int wrap,
                 int line_height, uint16_t color, const char* utf8);
int kg_sprite(kg_canvas* c, const ksp_pack* pack, const ksp_sprite* sprite, int x, int y);
int kg_sprite_scaled(kg_canvas* c, const ksp_pack* pack, const ksp_sprite* sprite, int x, int y,
                     unsigned width, unsigned height);
void kg_indexed(kg_canvas* c, int x, int y, unsigned width, unsigned height, const uint8_t* indices,
                const uint8_t* palette, unsigned colors);
void kg_indexed_scaled(kg_canvas* c, int x, int y, unsigned width, unsigned height,
                       const uint8_t* indices, unsigned source_width, unsigned source_height,
                       const uint8_t* palette, unsigned colors);
#endif
