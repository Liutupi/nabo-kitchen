#include <limits.h>
#include <string.h>
#include "kitchen_ui.h"

unsigned ku_fridge_matches(const ku_ui* u, uint16_t* out, unsigned capacity) {
    if (!u || (!out && capacity))
        return UINT_MAX;
    struct match {
        uint16_t index, score;
    } matches[10];
    unsigned count = 0;
    for (unsigned i = 0; i < u->catalog->count; ++i) {
        const kr_recipe* r = &u->catalog->items[i].recipe;
        if (r->ingredients & u->excluded)
            continue;
        unsigned needed = 0, missing = 0;
        if (r->repeated_main) {
            kc_ingredient ingredients[10];
            unsigned n;
            if (!kc_read_ingredients(u->pack, i, ingredients, 10, &n))
                return UINT_MAX;
            for (unsigned j = 0; j < n; ++j)
                if (ingredients[j].main) {
                    ++needed;
                    missing += !(u->fridge & (UINT64_C(1) << ingredients[j].id));
                }
        } else {
            uint64_t m = r->main_ingredients;
            while (m) {
                needed += (unsigned)(m & 1u);
                m >>= 1;
            }
            m = r->main_ingredients & ~u->fridge;
            while (m) {
                missing += (unsigned)(m & 1u);
                m >>= 1;
            }
        }
        if (missing > 1)
            continue;
        unsigned score = needed ? (needed - missing) * 1000 / needed : 1000, pos = 0;
        /* Catalog order is KRC ID-sorted; ties preserve that order. */
        while (pos < count && matches[pos].score >= score)
            ++pos;
        if (pos >= 10)
            continue;
        if (count < 10)
            ++count;
        for (unsigned j = count - 1; j > pos; --j)
            matches[j] = matches[j - 1];
        matches[pos] = (struct match){(uint16_t)i, (uint16_t)score};
    }
    unsigned n = count < capacity ? count : capacity;
    for (unsigned i = 0; i < n; ++i)
        out[i] = matches[i].index;
    return count;
}
unsigned ku_book_list(const ku_ui* u, uint16_t* out, unsigned capacity) {
    if (!u || (!out && capacity))
        return UINT_MAX;
    unsigned count = 0;
    for (unsigned i = 0; i < u->catalog->count; ++i) {
        int cooked = 0;
        for (unsigned j = 0; j < u->store.cooked_count; ++j)
            if (u->store.cooked[j].recipe == i) {
                cooked = 1;
                break;
            }
        if (u->book_filter == 1 && !cooked)
            continue;
        if (u->book_filter == 2 && !(u->favorites & (UINT64_C(1) << i)))
            continue;
        if (u->book_filter == 3)
            continue; /* Curated catalog has no user entries yet. */
        if (count < capacity)
            out[count] = (uint16_t)i;
        ++count;
    }
    return count;
}
unsigned ku_shopping_order(const ku_ui* u, uint8_t* out, unsigned capacity) {
    if (!u || (!out && capacity))
        return UINT_MAX;
    unsigned count = 0;
    for (unsigned bought = 0; bought < 2; ++bought)
        for (unsigned i = 0; i < u->shopping_count; ++i)
            if (!!u->shopping[i].bought == bought) {
                if (count < capacity)
                    out[count] = (uint8_t)i;
                ++count;
            }
    return count;
}
int ku_add_shopping(ku_ui* u, unsigned recipe) {
    if (!u || recipe >= u->catalog->count)
        return 0;
    kc_ingredient source[10];
    unsigned n;
    if (!kc_read_ingredients(u->pack, recipe, source, 10, &n))
        return 0;
    ku_shopping staged[KU_SHOPPING_CAPACITY];
    memcpy(staged, u->shopping, sizeof(staged));
    unsigned count = u->shopping_count;
    uint32_t next = 1;
    for (unsigned i = 0; i < count; ++i) {
        if (staged[i].uid == UINT32_MAX)
            return 0;
        if (staged[i].uid >= next)
            next = staged[i].uid + 1;
    }
    unsigned servings = u->catalog->items[recipe].servings;
    if (!servings)
        return 0;
    for (unsigned i = 0; i < n; ++i) {
        uint64_t qty =
            ((uint64_t)source[i].qty100 * u->conditions.people + servings / 2) / servings;
        if (qty > UINT32_MAX)
            return 0;
        unsigned j = 0;
        while (j < count && (staged[j].id != source[i].id || staged[j].unit != source[i].unit ||
                             staged[j].bought))
            ++j;
        if (j < count) {
            if (qty > UINT32_MAX - staged[j].qty100)
                return 0;
            staged[j].qty100 += (uint32_t)qty;
        } else {
            if (count >= KU_SHOPPING_CAPACITY || !next)
                return 0;
            staged[count++] = (ku_shopping){next++, (uint32_t)qty, source[i].id, source[i].unit, 0};
        }
    }
    memcpy(u->shopping, staged, sizeof(staged));
    u->shopping_count = (uint8_t)count;
    u->dirty = 1;
    return 1;
}
