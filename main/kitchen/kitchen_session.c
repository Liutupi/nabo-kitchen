#include "kitchen_session.h"
#include <limits.h>
#include <string.h>

void ks_init(ks_store* s) { memset(s, 0, sizeof(*s)); }
void ks_reset_session(ks_session* s) { memset(s, 0, sizeof(*s)); }

ks_result ks_complete_step(ks_store* store, ks_session* s, uint16_t recipe, unsigned step_count,
                           int32_t day) {
    (void)day;
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
        store->cooked[i] = (ks_cooked){recipe, 0, 0};
        ++store->cooked_count;
    }
    ++store->cooked[i].count;
    store->cooked[i].last_day = 0;
    if (store->history_count == KS_HISTORY_SIZE) {
        memmove(store->history, store->history + 1,
                (KS_HISTORY_SIZE - 1) * sizeof(store->history[0]));
        --store->history_count;
    }
    store->history[store->history_count++] = (ks_record){recipe, 0};
    s->completed = 1;
    return KS_COMPLETE;
}
unsigned ks_recent_age(const ks_store* s, unsigned recipe) {
    if (!s || s->history_count > KS_HISTORY_SIZE) return 99999;
    for (unsigned i = s->history_count; i > 0; --i)
        if (s->history[i-1].recipe == recipe) return s->history_count-i;
    return 99999;
}
ks_stats ks_get_stats(const ks_store* s, int32_t unused) {
    (void)unused;
    ks_stats out = {0};
    if (!s || s->history_count > KS_HISTORY_SIZE || s->cooked_count > KS_MAX_RECIPES) return out;
    out.unique = s->cooked_count;
    for (unsigned i = 0; i < s->cooked_count; ++i) {
        uint64_t sum = (uint64_t)out.total + s->cooked[i].count;
        out.total = sum > UINT32_MAX ? UINT32_MAX : (uint32_t)sum;
    }
    out.level = out.unique >= 100 && out.total >= 200 ? 4
              : out.unique >= 40 || out.total >= 60 ? 3
              : out.unique >= 10 ? 2 : 1;
    return out;
}
