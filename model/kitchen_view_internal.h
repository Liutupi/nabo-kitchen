#ifndef KITCHEN_VIEW_INTERNAL_H
#define KITCHEN_VIEW_INTERNAL_H
#include "kitchen_metadata.h"
#include "kitchen_view.h"
#define RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#define BG RGB(29, 26, 48)
#define PANEL RGB(44, 40, 69)
#define INK RGB(21, 18, 42)
#define PAPER RGB(251, 241, 227)
#define PAPER2 RGB(243, 228, 207)
#define GOLD RGB(255, 180, 94)
#define WHITE RGB(255, 255, 255)
#define DIM RGB(142, 136, 170)
#define RED RGB(230, 81, 82)
#define TOMATO RGB(255, 107, 91)
#define BLUE RGB(93, 183, 242)
#define GREEN RGB(79, 201, 138)
#define YELLOW RGB(255, 201, 63)
void kv_label(kg_canvas* c, const kv_scene* s, const kfn_font* font, int x, int y, int width,
              int height, uint16_t color, const char* text);
void kv_icon(kg_canvas* c, const kv_scene* s, const char* id, int x, int y, unsigned w, unsigned h);
void kv_food(kg_canvas* c, const kv_scene* s, unsigned recipe, int x, int y, unsigned w,
             unsigned h);
void kv_portrait(kg_canvas* c, const kv_scene* s, int x, int y, unsigned size);
void kv_header(kg_canvas* c, const kv_scene* s, const char* title, uint16_t accent);
void kv_footer(kg_canvas* c, const kv_scene* s, const char* text);
void kv_home_draw(kg_canvas* c, const kv_scene* s);
void kv_cooking_draw(kg_canvas* c, const kv_scene* s);
void kv_lists_draw(kg_canvas* c, const kv_scene* s);
#endif
