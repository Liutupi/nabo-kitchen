#ifndef KITCHEN_METADATA_H
#define KITCHEN_METADATA_H
#include <stdint.h>
typedef struct {
    const char *id, *name, *category;
    uint8_t version;
} km_ingredient;
extern const uint32_t km_krc_crc, km_krc_size;
extern const km_ingredient km_ingredients[];
extern const unsigned km_ingredient_count;
extern const uint8_t km_ingredient_order[], km_fridge_order[];
extern const unsigned km_ingredient_order_count, km_fridge_order_count;
extern const char *const km_assets[],
    *const km_vessels[], *const km_sauces[], *const km_garnishes[],
                                                 *const km_methods[],
                                                     *const km_heats[], *const km_seasoning_names[];
extern const unsigned km_assets_count, km_vessels_count, km_sauces_count, km_garnishes_count,
    km_methods_count, km_heats_count, km_seasoning_count;
#endif
