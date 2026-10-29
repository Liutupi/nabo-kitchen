#ifndef KITCHEN_UI_H
#define KITCHEN_UI_H
#include "kitchen_catalog.h"
#include "kitchen_input.h"
#include "kitchen_session.h"
#include "kitchen_timer.h"

/* P0 uses one monotonic clock for animation and the startup transition. */
#define KU_BOOT_DURATION_MS 3600

typedef struct {
    uint8_t people, minutes, combo;
} ku_conditions;
typedef struct {
    uint8_t page, focus;
    uint16_t recipe;
} ku_location;
typedef struct {
    uint32_t uid, qty100;
    uint8_t id, unit, bought;
} ku_shopping;
#define KU_SHOPPING_CAPACITY 100
typedef enum {
    KU_NO_MESSAGE,
    KU_NO_RECIPES,
    KU_NO_COMBO,
    KU_NO_REROLLS,
    KU_NO_TIMER_SLOT,
    KU_STEP_INCOMPLETE,
    KU_DATA_ERROR,
    KU_CAPACITY_ERROR,
    KU_STORE_ERROR,
    KU_CLOCK_LOST,
    KU_RECOVERED
} ku_message;
typedef struct {
    const kc_catalog* catalog;
    const krc_pack* pack;
    ks_store store;
    ks_session sessions[KR_MAX_RECIPES];
    uint16_t timer_steps[KR_MAX_RECIPES];
    kt_service timers;
    uint16_t timer_recipe[3];
    uint8_t timer_step[3];
    uint32_t timer_ids[3];
    uint64_t favorites, wanted, excluded, fridge;
    ku_shopping shopping[KU_SHOPPING_CAPACITY];
    uint8_t shopping_count;
    ku_conditions conditions, saved_conditions;
    uint16_t result[4], saved_result[4];
    uint8_t page, focus, result_count, result_focus, saved_count, notes, saved_notes;
    uint8_t difficulty, reroll_limit, rerolls_used, sound, volume;
    uint8_t outside, dirty, has_saved, stack_count, ingredient_count, fridge_count;
    uint8_t book_filter, settings_mode, clear_stage, less_spicy, less_oil;
    uint16_t step_scroll, step_scroll_max, paper_scroll, paper_scroll_max;
    uint8_t ingredient_order[64], fridge_order[64];
    ku_location stack[16];
    uint16_t recipe;
    int32_t day;
    uint8_t month;
    uint32_t daily_seed;
    int64_t transition_due, message_due;
    ku_message message;
} ku_ui;
/* Caller must keep catalog and pack alive. Ingredient order uses KRC ordinals. */
int ku_init(ku_ui* ui, const kc_catalog* catalog, const krc_pack* pack, int32_t day, unsigned month,
            uint32_t seed, const uint8_t* order, unsigned ingredients, const uint8_t* fridge_order,
            unsigned fridge_count);
void ku_set_day(ku_ui* ui, int32_t day, unsigned month, uint32_t seed);
void ku_tick(ku_ui* ui, int64_t now);
void ku_startup(ku_ui* ui, int64_t now);
void ku_key(ku_ui* ui, ki_key key, ki_kind kind, int64_t now);
/* Renderer reports measured scroll extent after laying out text/paper. */
void ku_scroll_extents(ku_ui* ui, unsigned step_max, unsigned paper_max);
/* Results use stable recipe indices, ordered as the prototype. */
unsigned ku_fridge_matches(const ku_ui* ui, uint16_t* indices, unsigned capacity);
unsigned ku_book_list(const ku_ui* ui, uint16_t* indices, unsigned capacity);
unsigned ku_shopping_order(const ku_ui* ui, uint8_t* indices, unsigned capacity);
int ku_add_shopping(ku_ui* ui, unsigned recipe);
#endif
