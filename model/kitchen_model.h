#ifndef KITCHEN_MODEL_H
#define KITCHEN_MODEL_H
#include <stdint.h>

/* One mulberry32 draw; state is updated for subsequent draws. */
uint32_t kitchen_random_next(uint32_t *state);
/* age is the number of days since the most recent eligible history entry. */
unsigned kitchen_recipe_weight(unsigned age, int favorite, int in_season,
                               int wanted, int above_difficulty);
#endif
