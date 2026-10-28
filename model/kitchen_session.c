#include "kitchen_session.h"
#include <limits.h>
#include <string.h>

void ks_init(ks_store* s) { memset(s, 0, sizeof(*s)); }
void ks_reset_session(ks_session* s) { memset(s, 0, sizeof(*s)); }

ks_result ks_complete_step(ks_store* store, ks_session* s, uint16_t recipe, unsigned step_count,
                           int32_t day) {
    if (!store || !s || !step_count || step_count > KS_MAX_STEPS || s->index >= step_count ||
        store->history_count > KS_HISTORY_SIZE || store->cooked_count > KS_MAX_RECIPES)
        return KS_INVALID;
    if (s->completed)
        return KS_ALREADY_COMPLETE;
    s->visited |= (uint16_t)(1u << s->index);
    if ((unsigned)s->index + 1 < step_count) {
        ++s->index;
        return KS_ADVANCED;
    }
    if ((s->visited & ((1u << step_count) - 1)) != (1u << step_count) - 1)
        return KS_INCOMPLETE;
    unsigned i = 0;
    while (i < store->cooked_count && store->cooked[i].recipe != recipe)
        ++i;
    if (i == store->cooked_count && i == KS_MAX_RECIPES)
        return KS_FULL;
    if (i < store->cooked_count && store->cooked[i].count == UINT32_MAX)
        return KS_FULL;
    if (i == store->cooked_count) {
        store->cooked[i] = (ks_cooked){recipe, 0, day};
        ++store->cooked_count;
    }
    ++store->cooked[i].count;
    store->cooked[i].last_day = day;
    if (store->history_count == KS_HISTORY_SIZE) {
        memmove(store->history, store->history + 1,
                (KS_HISTORY_SIZE - 1) * sizeof(store->history[0]));
        --store->history_count;
    }
    store->history[store->history_count++] = (ks_record){recipe, day};
    int64_t age = store->has_streak ? (int64_t)day - store->streak_last_day : INT64_MAX;
    if (age == 1 && store->streak_days < UINT32_MAX)
        ++store->streak_days;
    else if (age != 0 && age != 1)
        store->streak_days = 1;
    store->streak_last_day = day;
    store->has_streak = 1;
    s->completed = 1;
    return KS_COMPLETE;
}
ks_stats ks_get_stats(const ks_store* s, int32_t today) {
    ks_stats out = {0};
    if (!s || s->history_count > KS_HISTORY_SIZE || s->cooked_count > KS_MAX_RECIPES)
        return out;
    out.unique = s->cooked_count;
    for (unsigned i = 0; i < s->cooked_count; ++i) {
        uint64_t sum = (uint64_t)out.total + s->cooked[i].count;
        out.total = sum > UINT32_MAX ? UINT32_MAX : (uint32_t)sum;
    }
    for (unsigned i = 0; i < s->history_count; ++i)
        out.today += s->history[i].day == today;
    if (s->has_streak) {
        int64_t age = (int64_t)today - s->streak_last_day;
        if (age >= 0 && age <= 1)
            out.streak = s->streak_days;
    } else {
        /* Prototype's legacy fallback: distinct dates ending today or yesterday. */
        int64_t cursor = today;
        int have_latest = 0;
        int32_t latest = INT32_MIN;
        for (unsigned i = 0; i < s->history_count; ++i) {
            if (!have_latest || s->history[i].day > latest)
                latest = s->history[i].day;
            have_latest = 1;
        }
        if (have_latest && (int64_t)latest == (int64_t)today - 1)
            --cursor;
        while (cursor >= INT32_MIN) {
            int found = 0;
            for (unsigned i = 0; i < s->history_count; ++i)
                found |= s->history[i].day == cursor;
            if (!found)
                break;
            ++out.streak;
            --cursor;
        }
    }
    out.level = out.unique >= 100 && out.streak >= 30  ? 4
                : out.unique >= 40 || out.streak >= 14 ? 3
                : out.unique >= 10                     ? 2
                                                       : 1;
    return out;
}
