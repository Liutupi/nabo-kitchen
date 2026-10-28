#ifndef KITCHEN_RECOMMEND_H
#define KITCHEN_RECOMMEND_H
#include <stddef.h>
#include <stdint.h>

/* IDs are represented by input indices; the caller supplies a unique, stable order.
 * Ingredient bits must include every ingredient, including non-main ingredients.
 * Method is the caller's canonical primary-method ID. Weight is precomputed. */
/* Bounded initial-library limit until target timing and larger libraries are tested. */
#define KR_MAX_RECIPES 192
#define KR_RECIPE_WORDS ((KR_MAX_RECIPES + 63) / 64)
#define KR_MEAT 1u
#define KR_VEG 2u
#define KR_SOUP 4u
#define KR_MAIN_RELAXED 1u
#define KR_METHOD_RELAXED 2u
#define KR_TIME_RELAXED 4u
typedef struct {
    uint64_t ingredients, main_ingredients;
    uint16_t minutes, weight;
    uint8_t courses, method, repeated_main;
} kr_recipe;
typedef enum { KR_OK = 0, KR_NO_ALLOWED = 1, KR_NO_COMBINATION = 2, KR_INVALID = 3 } kr_status;
typedef struct {
    kr_status status;
    uint16_t indices[4];
    uint8_t count, notes;
} kr_result;

/* minutes==0 means unlimited; avoid is the previously selected set. No heap. */
kr_result kitchen_recommend(const kr_recipe* recipes, size_t count, uint64_t excluded,
                            unsigned combo, unsigned minutes, uint32_t seed, const uint16_t* avoid,
                            size_t avoid_count);
#endif
