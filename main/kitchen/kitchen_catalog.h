#ifndef KITCHEN_CATALOG_H
#define KITCHEN_CATALOG_H
#include "kitchen_krc.h"
#include "kitchen_recommend.h"
typedef struct {
    kr_recipe recipe;
    uint32_t hash;
    char id[41], name[41]; /* IDs: 40 ASCII bytes; names: up to 10 UTF-8 scalars. */
    uint16_t season_mask, tags;
    uint8_t servings, difficulty;
} kc_item;
typedef struct {
    kc_item* items;
    unsigned capacity, count;
} kc_catalog;
typedef struct {
    char text[193], tip[65];
    uint32_t timer_seconds;
    uint8_t method, heat;
} kc_step;
typedef struct {
    uint32_t qty100;
    uint8_t id, unit, main;
} kc_ingredient;
typedef struct {
    char quantity[33];
    uint8_t id;
} kc_seasoning;
typedef struct {
    uint8_t vessel, layer_count, garnish_count, hero_required;
    int16_t sauce;
    uint16_t layers[4], garnishes[3];
} kc_dish;
/* Caller supplies item storage for pack->count entries. Converts only after full
 * validation; requires cJSON and may allocate transiently.
 * Caller must use a worker task and cannot assume the data survives if it fails. */
int kc_load(kc_catalog* out, const krc_pack* pack);
/* Same validation and result; call progress after each validated recipe. A
 * device worker can block for a scheduler tick here without adding RTOS to C. */
int kc_load_cooperative(kc_catalog* out, const krc_pack* pack,
                        void (*progress)(void*), void* context);
/* Fetch a step on demand. The fixed buffers hold up to 48/16 four-byte scalars. */
int kc_read_step(const krc_pack* pack, unsigned recipe, unsigned step, kc_step* out,
                 unsigned* step_count);
/* Quantities use hundredths. Reject unsupported precision instead of truncating. */
int kc_read_ingredients(const krc_pack* pack, unsigned recipe, kc_ingredient* out,
                        unsigned capacity, unsigned* count);
int kc_read_seasonings(const krc_pack* pack, unsigned recipe, kc_seasoning* out, unsigned capacity,
                       unsigned* count);
int kc_read_dish(const krc_pack* pack, unsigned recipe, kc_dish* out);
#endif
