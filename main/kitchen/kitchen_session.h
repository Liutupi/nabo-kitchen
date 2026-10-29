#ifndef KITCHEN_SESSION_H
#define KITCHEN_SESSION_H
#include <stdint.h>

/* Legacy date fields retain layout only; v2 completion order is the history array. */
#define KS_MAX_STEPS 12
#define KS_MAX_RECIPES 192
#define KS_HISTORY_SIZE 60
typedef struct {
    uint16_t recipe;
    int32_t day;
} ks_record;
typedef struct {
    uint16_t recipe;
    uint32_t count;
    int32_t last_day;
} ks_cooked;
typedef struct {
    ks_record history[KS_HISTORY_SIZE];
    ks_cooked cooked[KS_MAX_RECIPES];
    uint8_t history_count, cooked_count, has_streak;
    int32_t streak_last_day;
    uint32_t streak_days;
} ks_store;
typedef struct {
    uint16_t visited;
    uint8_t index, completed;
} ks_session;
typedef enum {
    KS_ADVANCED,
    KS_COMPLETE,
    KS_ALREADY_COMPLETE,
    KS_INCOMPLETE,
    KS_INVALID,
    KS_FULL
} ks_result;
typedef struct {
    uint32_t unique, total, streak, today, level;
} ks_stats;

unsigned ks_recent_age(const ks_store* store, unsigned recipe);
void ks_init(ks_store* store);
void ks_reset_session(ks_session* session);
ks_result ks_complete_step(ks_store* store, ks_session* session, uint16_t recipe,
                           unsigned step_count, int32_t day);
ks_stats ks_get_stats(const ks_store* store, int32_t today);
#endif
