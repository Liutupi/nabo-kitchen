#include "kitchen_recommend.h"
#include "kitchen_model.h"

typedef struct {
    const kr_recipe* recipes;
    size_t count;
    uint64_t excluded;
    unsigned minutes, tier, length;
    uint8_t roles[4];
    uint64_t eligible[4][KR_RECIPE_WORDS];
    int bounded;
    uint16_t selected[4];
    const uint16_t* avoid;
    size_t avoid_count;
    int avoid_filter, choose;
    uint64_t sum, pick;
    kr_result result;
} search;

/* Prototype deduplicates equal ID sets before applying soft constraints. Keep
 * their earliest role-compatible permutation in source traversal order. */
static int earlier_permutation(const search* s, unsigned depth, uint8_t used, int order) {
    if (depth == s->length)
        return order < 0;
    for (unsigned i = 0; i < s->length; ++i) {
        if ((used & (1u << i)) || !(s->recipes[s->selected[i]].courses & s->roles[depth]) ||
            (s->bounded && !(s->eligible[depth][s->selected[i] / 64] &
                             (UINT64_C(1) << (s->selected[i] % 64)))))
            continue;
        int next = order;
        if (!next && s->selected[i] != s->selected[depth])
            next = s->selected[i] < s->selected[depth] ? -1 : 1;
        if (next <= 0 && earlier_permutation(s, depth + 1, used | (1u << i), next))
            return 1;
    }
    return 0;
}
static int same_avoid(const search* s) {
    if (s->avoid_count != s->length)
        return 0;
    for (unsigned i = 0; i < s->length; ++i) {
        int found = 0;
        for (size_t j = 0; j < s->avoid_count; ++j)
            found |= s->selected[i] == s->avoid[j];
        if (!found)
            return 0;
    }
    return 1;
}
static unsigned violations(const search* s) {
    uint64_t main = 0;
    unsigned notes = 0, total = 0;
    int same_method = s->length > 1;
    for (unsigned i = 0; i < s->length; ++i) {
        const kr_recipe* r = &s->recipes[s->selected[i]];
        if (s->length > 1 && ((main & r->main_ingredients) || r->repeated_main))
            notes |= KR_MAIN_RELAXED;
        main |= r->main_ingredients;
        if (r->method != s->recipes[s->selected[0]].method)
            same_method = 0;
        total += r->minutes;
    }
    if (same_method)
        notes |= KR_METHOD_RELAXED;
    if (s->minutes && total > s->minutes)
        notes |= KR_TIME_RELAXED;
    return notes;
}
static void visit(search* s) {
    unsigned notes = violations(s);
    unsigned allowed = s->tier == 0   ? 0
                       : s->tier == 1 ? KR_MAIN_RELAXED
                       : s->tier == 2 ? KR_MAIN_RELAXED | KR_METHOD_RELAXED
                                      : 7;
    if ((notes & ~allowed) || (s->avoid_filter && same_avoid(s)))
        return;
    if (s->length > 1 && earlier_permutation(s, 0, 0, 0))
        return;
    uint64_t weight = 0;
    for (unsigned i = 0; i < s->length; ++i)
        weight += s->recipes[s->selected[i]].weight;
    if (s->choose) {
        if (s->pick < weight && s->result.status != KR_OK) {
            s->result.status = KR_OK;
            s->result.count = (uint8_t)s->length;
            s->result.notes = (uint8_t)notes;
            for (unsigned i = 0; i < s->length; ++i)
                s->result.indices[i] = s->selected[i];
        } else if (s->result.status != KR_OK)
            s->pick -= weight;
    } else
        s->sum += weight;
}
#ifdef KR_TEST_TRACE
size_t kr_test_visits;
#endif
static void enumerate(search* s, unsigned depth) {
#ifdef KR_TEST_TRACE
    ++kr_test_visits;
#endif
    if (s->choose && s->result.status == KR_OK)
        return;
    if (depth == s->length) {
        visit(s);
        return;
    }
    for (size_t i = 0; i < s->count; ++i) {
        const kr_recipe* r = &s->recipes[i];
        if (s->bounded && !(s->eligible[depth][i / 64] & (UINT64_C(1) << (i % 64))))
            continue;
        if ((r->ingredients & s->excluded) || (s->roles[depth] && !(r->courses & s->roles[depth])))
            continue;
        int duplicate = 0;
        for (unsigned j = 0; j < depth; ++j)
            duplicate |= s->selected[j] == i;
        if (duplicate)
            continue;
        /* These constraints are monotonic for partial groups, so pruning them
         * does not change which complete ID set appears first in the prototype. */
        if (s->tier < 3 && s->minutes) {
            unsigned total = r->minutes;
            for (unsigned j = 0; j < depth; ++j)
                total += s->recipes[s->selected[j]].minutes;
            if (total > s->minutes)
                continue;
        }
        if (s->tier == 0 && s->length > 1) {
            if (r->repeated_main)
                continue;
            uint64_t prior = 0;
            for (unsigned j = 0; j < depth; ++j)
                prior |= s->recipes[s->selected[j]].main_ingredients;
            if (prior & r->main_ingredients)
                continue;
        }
        s->selected[depth] = (uint16_t)i;
        enumerate(s, depth + 1);
    }
}
kr_result kitchen_recommend(const kr_recipe* recipes, size_t count, uint64_t excluded,
                            unsigned combo, unsigned minutes, uint32_t seed, const uint16_t* avoid,
                            size_t avoid_count) {
#ifdef KR_TEST_TRACE
    kr_test_visits = 0;
#endif
    kr_result bad = {.status = KR_INVALID};
    if ((!recipes && count) || count > KR_MAX_RECIPES || combo > 3 || avoid_count > 4 ||
        (!avoid && avoid_count))
        return bad;
    for (size_t i = 0; i < count; ++i)
        if (!recipes[i].weight || (recipes[i].main_ingredients & ~recipes[i].ingredients))
            return bad;
    for (size_t i = 0; i < avoid_count; ++i) {
        if (avoid[i] >= count)
            return bad;
        for (size_t j = 0; j < i; ++j)
            if (avoid[i] == avoid[j])
                return bad;
    }
    search s = {.recipes = recipes,
                .count = count,
                .excluded = excluded,
                .minutes = minutes,
                .length = combo ? combo + 1 : 1,
                .avoid = avoid,
                .avoid_count = avoid_count,
                .result = {.status = KR_NO_ALLOWED}};
    if (combo == 1) {
        s.roles[0] = KR_MEAT;
        s.roles[1] = KR_VEG;
    }
    if (combo == 2) {
        s.roles[0] = KR_MEAT;
        s.roles[1] = KR_VEG;
        s.roles[2] = KR_SOUP;
    }
    if (combo == 3) {
        s.roles[0] = KR_MEAT;
        s.roles[1] = KR_MEAT;
        s.roles[2] = KR_VEG;
        s.roles[3] = KR_SOUP;
    }
    /* Larger catalogs use a bounded, seeded reservoir per role. Each role
     * retains all candidates when it has at most twelve; otherwise twelve
     * randomly selected hard-compatible candidates. Repeated meat roles use
     * the same pool. Sampling limits worst-case search work independently of
     * catalog size; weights remain exact within the selected pools. */
    if (count > 64 && combo) {
        s.bounded = 1;
        for (unsigned depth = 0; depth < s.length; ++depth) {
            uint16_t reservoir[12];
            unsigned seen = 0;
            uint32_t state = seed ^ (UINT32_C(0x9e3779b9) * (s.roles[depth] + 1u));
            for (unsigned i = 0; i < count; ++i) {
                if ((recipes[i].ingredients & excluded) ||
                    !(recipes[i].courses & s.roles[depth])) continue;
                ++seen;
                unsigned slot = seen <= 12 ? seen - 1 : kitchen_random_next(&state) % seen;
                if (slot < 12) reservoir[slot] = (uint16_t)i;
            }
            unsigned n = seen < 12 ? seen : 12;
            for (unsigned i = 0; i < n; ++i)
                s.eligible[depth][reservoir[i] / 64] |= UINT64_C(1) << (reservoir[i] % 64);
        }
    }
    int any = 0;
    for (size_t i = 0; i < count; ++i)
        any |= !(recipes[i].ingredients & excluded);
    if (!any)
        return s.result;
    s.result.status = KR_NO_COMBINATION;
    for (s.tier = 0; s.tier < 4; ++s.tier) {
        s.sum = 0;
        s.avoid_filter = 1;
        enumerate(&s, 0);
        if (!s.sum) {
            s.avoid_filter = 0;
            enumerate(&s, 0);
        }
        if (!s.sum)
            continue;
        s.pick = (uint64_t)kitchen_random_next(&seed) % s.sum;
        s.choose = 1;
        enumerate(&s, 0);
        return s.result;
    }
    return s.result;
}
