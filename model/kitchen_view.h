#ifndef KITCHEN_VIEW_H
#define KITCHEN_VIEW_H
#include "kitchen_canvas.h"
#include "kitchen_kan.h"
#include "kitchen_ui.h"
typedef struct {
    const kfn_pack* fonts;
    const ksp_pack *dish, *icons, *ui;
    const kan_pack* animation;
    kfn_font han12, han15, han16, han18, han20, latin12, latin34;
    kc_dish dishes[KR_MAX_RECIPES];
    unsigned dish_count;
} kv_assets;
typedef struct {
    ku_ui* state;
    const kv_assets* assets;
    int64_t now;
    int battery;
    char clock[6];
    const uint8_t* animation_frame;
    kc_step step;
    unsigned step_count;
    kc_ingredient ingredients[10];
    unsigned ingredient_count;
    kc_seasoning seasonings[12];
    unsigned seasoning_count;
    uint16_t fridge_matches[10], book[KR_MAX_RECIPES];
    unsigned match_count, book_count;
    uint8_t shopping[KU_SHOPPING_CAPACITY];
    unsigned shopping_count;
} kv_scene;
/* Decode recipe metadata in the worker before calling the stripe renderer. */
int kv_assets_init(kv_assets* out, const kc_catalog* catalog, const krc_pack* recipes,
                   const kfn_pack* fonts, const ksp_pack* dish, const ksp_pack* icons,
                   const ksp_pack* ui, const kan_pack* animation);
int kv_prepare(kv_scene* scene, ku_ui* state, const kv_assets* assets, int64_t now, int battery,
               const char* clock, const uint8_t* animation_frame);
int kv_draw(kg_canvas* canvas, const kv_scene* scene);
#endif
