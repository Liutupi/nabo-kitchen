#include "kitchen_model.h"

uint32_t kitchen_random_next(uint32_t *state) {
    uint32_t a = *state += UINT32_C(0x6D2B79F5);
    uint32_t t = (a ^ (a >> 15)) * (a | 1u);
    t ^= t + ((t ^ (t >> 7)) * (t | 61u));
    return t ^ (t >> 14);
}
unsigned kitchen_recipe_weight(unsigned age, int favorite, int in_season,
                               int wanted, int above_difficulty) {
    unsigned w = 100;
    w = w * (age < 3 ? 10 : age < 7 ? 40 : age < 14 ? 80 : 100) / 100;
    if (favorite) w = w * 150 / 100;
    if (in_season) w = w * 130 / 100;
    if (wanted) w = w * 120 / 100;
    if (above_difficulty) w = w * 50 / 100;
    return w ? w : 1;
}
