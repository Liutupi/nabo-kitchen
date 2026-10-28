#include "kitchen_save.h"
#include <limits.h>
#include <string.h>
#include "kitchen_metadata.h"
typedef struct {
    uint8_t* data;
    size_t cap, pos;
    int ok;
} writer;
typedef struct {
    const uint8_t* data;
    size_t size, pos;
    int ok;
} reader;
static uint32_t crc32(const uint8_t* p, size_t n) {
    uint32_t c = UINT32_MAX;
    for (size_t i = 0; i < n; ++i) {
        c ^= p[i];
        for (unsigned j = 0; j < 8; ++j)
            c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1u)));
    }
    return ~c;
}
static void put(writer* w, uint64_t v, unsigned n) {
    if (!w->ok || n > w->cap - w->pos) {
        w->ok = 0;
        return;
    }
    for (unsigned i = 0; i < n; ++i)
        w->data[w->pos++] = (uint8_t)(v >> (i * 8));
}
static uint64_t get(reader* r, unsigned n) {
    if (!r->ok || n > r->size - r->pos) {
        r->ok = 0;
        return 0;
    }
    uint64_t v = 0;
    for (unsigned i = 0; i < n; ++i)
        v |= (uint64_t)r->data[r->pos++] << (i * 8);
    return v;
}
int ku_save_header(const uint8_t* buffer, size_t length, uint16_t* version, uint32_t* sequence,
                   uint32_t* crc) {
    if (!buffer || length < 20 || length > KU_SAVE_MAX)
        return 0;
    reader r = {buffer, length, 0, 1};
    if (get(&r, 4) != 0x3154534bu)
        return 0;
    uint16_t v = (uint16_t)get(&r, 2);
    if (get(&r, 2) != 0 || get(&r, 4) != length)
        return 0;
    uint32_t checksum = (uint32_t)get(&r, 4), seq = (uint32_t)get(&r, 4);
    if (crc32(buffer + 20, length - 20) != checksum)
        return 0;
    if (version)
        *version = v;
    if (sequence)
        *sequence = seq;
    if (crc)
        *crc = checksum;
    return 1;
}
static uint32_t hash(const char* s) {
    uint32_t h = 2166136261u;
    while (*s) {
        h ^= (unsigned char)*s++;
        h *= 16777619u;
    }
    return h;
}
static int recipe_for(const ku_ui* u, uint32_t h) {
    int found = -1;
    for (unsigned i = 0; i < u->catalog->count; ++i)
        if (u->catalog->items[i].hash == h) {
            if (found >= 0)
                return -1;
            found = (int)i;
        }
    return found;
}
static int ingredient_for(uint32_t h) {
    int found = -1;
    for (unsigned i = 0; i < km_ingredient_count; ++i)
        if (hash(km_ingredients[i].id) == h) {
            if (found >= 0)
                return -1;
            found = (int)i;
        }
    return found;
}
static void conditions(writer* w, ku_conditions c) {
    put(w, c.people, 1);
    put(w, c.minutes, 1);
    put(w, c.combo, 1);
}
static ku_conditions read_conditions(reader* r) {
    ku_conditions c = {(uint8_t)get(r, 1), (uint8_t)get(r, 1), (uint8_t)get(r, 1)};
    if (c.people < 1 || c.people > 5 ||
        (c.minutes != 0 && c.minutes != 15 && c.minutes != 30 && c.minutes != 60) || c.combo > 3)
        r->ok = 0;
    return c;
}
static void recipe_mask(writer* w, const ku_ui* u, const uint64_t bits[KR_RECIPE_WORDS]) {
    unsigned n = 0;
    for (unsigned i = 0; i < u->catalog->count; ++i)
        n += (unsigned)ku_recipe_bit(bits, i);
    put(w, n, 1);
    for (unsigned i = 0; i < u->catalog->count; ++i)
        if (ku_recipe_bit(bits, i))
            put(w, u->catalog->items[i].hash, 4);
}
static void ingredient_mask(writer* w, uint64_t bits) {
    unsigned n = 0;
    for (unsigned i = 0; i < km_ingredient_count; ++i)
        n += (unsigned)((bits >> i) & 1u);
    put(w, n, 1);
    for (unsigned i = 0; i < km_ingredient_count; ++i)
        if ((bits >> i) & 1u)
            put(w, hash(km_ingredients[i].id), 4);
}
size_t ku_save_encode(const ku_ui* u, uint8_t* buffer, size_t cap, uint32_t sequence, int64_t mono,
                      int64_t wall) {
    if (!u || !u->catalog || !u->catalog->items || !buffer || cap < 20 || cap > KU_SAVE_MAX ||
        mono < 0 || u->catalog->count > KR_MAX_RECIPES || km_ingredient_count > 64 ||
        u->store.history_count > 60 || u->store.cooked_count > KS_MAX_RECIPES ||
        u->shopping_count > KU_SHOPPING_CAPACITY || u->timers.count > 3)
        return 0;
    for (unsigned i = u->catalog->count; i < KR_MAX_RECIPES; ++i)
        if (ku_recipe_bit(u->favorites, i) || ku_recipe_bit(u->wanted, i))
            return 0;
    if (km_ingredient_count < 64 && ((u->excluded | u->fridge) >> km_ingredient_count))
        return 0;
    writer w = {buffer, cap, 0, 1};
    put(&w, 0x3154534bu, 4);
    put(&w, 1, 2);
    put(&w, 0, 2);
    put(&w, 0, 4);
    put(&w, 0, 4);
    put(&w, sequence, 4);
    conditions(&w, u->conditions);
    put(&w, u->difficulty, 1);
    put(&w, u->reroll_limit, 1);
    put(&w, u->sound, 1);
    put(&w, u->volume, 1);
    put(&w, u->less_spicy, 1);
    put(&w, u->less_oil, 1);
    put(&w, (uint32_t)u->day, 4);
    put(&w, u->month, 1);
    put(&w, u->daily_seed, 4);
    put(&w, u->rerolls_used, 1);
    put(&w, u->has_saved, 1);
    conditions(&w, u->has_saved ? u->saved_conditions : u->conditions);
    put(&w, u->saved_notes, 1);
    put(&w, u->saved_count, 1);
    for (unsigned i = 0; i < u->saved_count; ++i) {
        if (i >= 4 || u->saved_result[i] >= u->catalog->count)
            return 0;
        put(&w, u->catalog->items[u->saved_result[i]].hash, 4);
    }
    put(&w, u->store.has_streak, 1);
    put(&w, (uint32_t)u->store.streak_last_day, 4);
    put(&w, u->store.streak_days, 4);
    put(&w, u->store.history_count, 1);
    for (unsigned i = 0; i < u->store.history_count; ++i) {
        const ks_record* x = &u->store.history[i];
        if (x->recipe >= u->catalog->count)
            return 0;
        put(&w, u->catalog->items[x->recipe].hash, 4);
        put(&w, (uint32_t)x->day, 4);
    }
    put(&w, u->store.cooked_count, 1);
    for (unsigned i = 0; i < u->store.cooked_count; ++i) {
        const ks_cooked* x = &u->store.cooked[i];
        if (x->recipe >= u->catalog->count)
            return 0;
        put(&w, u->catalog->items[x->recipe].hash, 4);
        put(&w, x->count, 4);
        put(&w, (uint32_t)x->last_day, 4);
    }
    unsigned sessions = 0;
    for (unsigned i = 0; i < u->catalog->count; ++i)
        if (u->sessions[i].visited || u->sessions[i].index || u->sessions[i].completed ||
            u->timer_steps[i])
            ++sessions;
    put(&w, sessions, 1);
    for (unsigned i = 0; i < u->catalog->count; ++i) {
        const ks_session* s = &u->sessions[i];
        if (!(s->visited || s->index || s->completed || u->timer_steps[i]))
            continue;
        put(&w, u->catalog->items[i].hash, 4);
        put(&w, s->visited, 2);
        put(&w, s->index, 1);
        put(&w, s->completed, 1);
        put(&w, u->timer_steps[i], 2);
    }
    recipe_mask(&w, u, u->favorites);
    recipe_mask(&w, u, u->wanted);
    ingredient_mask(&w, u->excluded);
    ingredient_mask(&w, u->fridge);
    put(&w, u->shopping_count, 1);
    for (unsigned i = 0; i < u->shopping_count; ++i) {
        const ku_shopping* s = &u->shopping[i];
        if (s->id >= km_ingredient_count)
            return 0;
        put(&w, s->uid, 4);
        put(&w, s->qty100, 4);
        put(&w, hash(km_ingredients[s->id].id), 4);
        put(&w, s->unit, 1);
        put(&w, s->bought, 1);
    }
    put(&w, u->timers.next_id, 4);
    put(&w, u->timers.count, 1);
    for (unsigned i = 0; i < u->timers.count; ++i) {
        const kt_timer* t = &u->timers.items[i];
        unsigned slot = 0;
        while (slot < 3 && u->timer_ids[slot] != t->id)
            ++slot;
        if (slot == 3 || u->timer_recipe[slot] >= u->catalog->count || t->total_ms < 0)
            return 0;
        int64_t left = kt_remaining(t, mono);
        int have_wall = wall >= 0 && left <= INT64_MAX - wall;
        put(&w, t->id, 4);
        put(&w, (uint64_t)t->total_ms, 8);
        put(&w, (uint64_t)left, 8);
        put(&w, t->status, 1);
        put(&w, t->acknowledged, 1);
        put(&w, u->catalog->items[u->timer_recipe[slot]].hash, 4);
        put(&w, u->timer_step[slot], 1);
        put(&w, have_wall, 1);
        put(&w, have_wall ? (uint64_t)(wall + left) : 0, 8);
    }
    if (!w.ok || w.pos > UINT32_MAX)
        return 0;
    uint32_t length = (uint32_t)w.pos, crc = crc32(buffer + 20, w.pos - 20);
    for (unsigned i = 0; i < 4; ++i) {
        buffer[8 + i] = (uint8_t)(length >> (8 * i));
        buffer[12 + i] = (uint8_t)(crc >> (8 * i));
    }
    return w.pos;
}
static int read_recipe_mask(reader* r, const ku_ui* u, uint64_t out[KR_RECIPE_WORDS]) {
    unsigned n = (unsigned)get(r, 1);
    if (n > KR_MAX_RECIPES) return 0;
    uint64_t bits[KR_RECIPE_WORDS] = {0};
    for (unsigned i = 0; i < n; ++i) {
        int index = recipe_for(u, (uint32_t)get(r, 4));
        if (index < 0 || ku_recipe_bit(bits, (unsigned)index)) return 0;
        ku_recipe_flip(bits, (unsigned)index);
    }
    memcpy(out, bits, sizeof bits);
    return r->ok;
}
static int read_ingredient_mask(reader* r, uint64_t* out) {
    unsigned n = (unsigned)get(r, 1);
    if (n > 64) return 0;
    uint64_t bits = 0;
    for (unsigned i = 0; i < n; ++i) {
        int index = ingredient_for((uint32_t)get(r, 4));
        if (index < 0 || (bits & (UINT64_C(1) << index))) return 0;
        bits |= UINT64_C(1) << index;
    }
    *out = bits;
    return r->ok;
}
int ku_save_decode(ku_ui* u, ku_ui* scratch, const uint8_t* buffer, size_t length,
                   uint32_t* sequence, int64_t mono, int64_t wall, int* clock_lost) {
    if (!u || !scratch || u == scratch || !u->catalog || !buffer || length < 20 ||
        length > KU_SAVE_MAX || mono < 0)
        return 0;
    reader r = {buffer, length, 0, 1};
    if (get(&r, 4) != 0x3154534bu || get(&r, 2) != 1 || get(&r, 2) != 0 || get(&r, 4) != length)
        return 0;
    uint32_t expected = (uint32_t)get(&r, 4), seq = (uint32_t)get(&r, 4);
    if (crc32(buffer + 20, length - 20) != expected)
        return 0;
    *scratch = *u;
    ku_ui* tmp = scratch;
    tmp->conditions = read_conditions(&r);
    tmp->difficulty = (uint8_t)get(&r, 1);
    tmp->reroll_limit = (uint8_t)get(&r, 1);
    tmp->sound = (uint8_t)get(&r, 1);
    tmp->volume = (uint8_t)get(&r, 1);
    tmp->less_spicy = (uint8_t)get(&r, 1);
    tmp->less_oil = (uint8_t)get(&r, 1);
    tmp->day = (int32_t)(uint32_t)get(&r, 4);
    tmp->month = (uint8_t)get(&r, 1);
    tmp->daily_seed = (uint32_t)get(&r, 4);
    tmp->rerolls_used = (uint8_t)get(&r, 1);
    tmp->has_saved = (uint8_t)get(&r, 1);
    tmp->saved_conditions = read_conditions(&r);
    tmp->saved_notes = (uint8_t)get(&r, 1);
    tmp->saved_count = (uint8_t)get(&r, 1);
    if (tmp->difficulty < 1 || tmp->difficulty > 3 || tmp->reroll_limit < 1 ||
        tmp->reroll_limit > 5 || tmp->sound > 1 || tmp->volume > 100 || tmp->less_spicy > 1 ||
        tmp->less_oil > 1 || tmp->month < 1 || tmp->month > 12 || tmp->has_saved > 1 ||
        tmp->saved_count > 4 || tmp->saved_notes > 7 || (!tmp->has_saved && tmp->saved_count) ||
        (tmp->has_saved && !tmp->saved_count))
        return 0;
    uint64_t seen[KR_RECIPE_WORDS] = {0};
    for (unsigned i = 0; i < tmp->saved_count; ++i) {
        int idx = recipe_for(tmp, (uint32_t)get(&r, 4));
        if (idx < 0 || ku_recipe_bit(seen, (unsigned)idx))
            return 0;
        ku_recipe_flip(seen, (unsigned)idx);
        tmp->saved_result[i] = (uint16_t)idx;
    }
    ks_init(&tmp->store);
    tmp->store.has_streak = (uint8_t)get(&r, 1);
    tmp->store.streak_last_day = (int32_t)(uint32_t)get(&r, 4);
    tmp->store.streak_days = (uint32_t)get(&r, 4);
    if (tmp->store.has_streak > 1)
        return 0;
    tmp->store.history_count = (uint8_t)get(&r, 1);
    if (tmp->store.history_count > 60)
        return 0;
    for (unsigned i = 0; i < tmp->store.history_count; ++i) {
        int idx = recipe_for(tmp, (uint32_t)get(&r, 4));
        int32_t day = (int32_t)(uint32_t)get(&r, 4);
        if (idx < 0)
            return 0;
        tmp->store.history[i] = (ks_record){(uint16_t)idx, day};
    }
    tmp->store.cooked_count = (uint8_t)get(&r, 1);
    if (tmp->store.cooked_count > KS_MAX_RECIPES)
        return 0;
    memset(seen, 0, sizeof seen);
    for (unsigned i = 0; i < tmp->store.cooked_count; ++i) {
        int idx = recipe_for(tmp, (uint32_t)get(&r, 4));
        uint32_t count = (uint32_t)get(&r, 4);
        int32_t day = (int32_t)(uint32_t)get(&r, 4);
        if (idx < 0 || !count || ku_recipe_bit(seen, (unsigned)idx))
            return 0;
        ku_recipe_flip(seen, (unsigned)idx);
        tmp->store.cooked[i] = (ks_cooked){(uint16_t)idx, count, day};
    }
    memset(tmp->sessions, 0, sizeof(tmp->sessions));
    memset(tmp->timer_steps, 0, sizeof(tmp->timer_steps));
    unsigned sessions = (unsigned)get(&r, 1);
    if (sessions > KR_MAX_RECIPES)
        return 0;
    memset(seen, 0, sizeof seen);
    for (unsigned i = 0; i < sessions; ++i) {
        int idx = recipe_for(tmp, (uint32_t)get(&r, 4));
        uint16_t visited = (uint16_t)get(&r, 2);
        uint8_t pos = (uint8_t)get(&r, 1), complete = (uint8_t)get(&r, 1);
        uint16_t timed = (uint16_t)get(&r, 2);
        if (idx < 0 || ku_recipe_bit(seen, (unsigned)idx) || pos >= 12 || complete > 1 ||
            (visited & 0xf000) || (timed & 0xf000))
            return 0;
        ku_recipe_flip(seen, (unsigned)idx);
        tmp->sessions[idx] = (ks_session){visited, pos, complete};
        tmp->timer_steps[idx] = timed;
    }
    if (!read_recipe_mask(&r, tmp, tmp->favorites) || !read_recipe_mask(&r, tmp, tmp->wanted) ||
        !read_ingredient_mask(&r, &tmp->excluded) || !read_ingredient_mask(&r, &tmp->fridge))
        return 0;
    tmp->shopping_count = (uint8_t)get(&r, 1);
    if (tmp->shopping_count > KU_SHOPPING_CAPACITY)
        return 0;
    for (unsigned i = 0; i < tmp->shopping_count; ++i) {
        uint32_t uid = (uint32_t)get(&r, 4), qty = (uint32_t)get(&r, 4);
        int id = ingredient_for((uint32_t)get(&r, 4));
        unsigned unit = (unsigned)get(&r, 1), bought = (unsigned)get(&r, 1);
        if (!uid || id < 0 || unit >= 14 || bought > 1)
            return 0;
        for (unsigned j = 0; j < i; ++j)
            if (tmp->shopping[j].uid == uid)
                return 0;
        tmp->shopping[i] = (ku_shopping){uid, qty, (uint8_t)id, (uint8_t)unit, (uint8_t)bought};
    }
    kt_init(&tmp->timers);
    tmp->timers.next_id = (uint32_t)get(&r, 4);
    tmp->timers.count = (uint32_t)get(&r, 1);
    if (!tmp->timers.next_id || tmp->timers.count > 3)
        return 0;
    memset(tmp->timer_ids, 0, sizeof(tmp->timer_ids));
    int lost = 0;
    for (unsigned i = 0; i < tmp->timers.count; ++i) {
        uint32_t id = (uint32_t)get(&r, 4);
        uint64_t total = get(&r, 8), left = get(&r, 8);
        unsigned status = (unsigned)get(&r, 1), ack = (unsigned)get(&r, 1);
        int recipe = recipe_for(tmp, (uint32_t)get(&r, 4));
        unsigned step = (unsigned)get(&r, 1), valid = (unsigned)get(&r, 1);
        uint64_t deadline = get(&r, 8);
        if (!id || id >= tmp->timers.next_id || !total || total > INT64_MAX || left > total ||
            status > KT_DONE || ack > 1 || recipe < 0 || step >= 12 || valid > 1 ||
            deadline > INT64_MAX)
            return 0;
        for (unsigned j = 0; j < i; ++j)
            if (tmp->timer_ids[j] == id)
                return 0;
        if (status == KT_DONE && left)
            return 0;
        if (status == KT_RUNNING) {
            if (valid && wall >= 0 && deadline >= left && (uint64_t)wall >= deadline - left)
                left = deadline > (uint64_t)wall ? deadline - (uint64_t)wall : 0;
            else {
                status = KT_PAUSED;
                lost = 1;
            }
        }
        if (left > total || left > (uint64_t)(INT64_MAX - mono))
            return 0;
        tmp->timers.items[i] = (kt_timer){
            id, (int64_t)total, (int64_t)left, mono + (int64_t)left, (kt_status)status, (int)ack};
        tmp->timer_ids[i] = id;
        tmp->timer_recipe[i] = (uint16_t)recipe;
        tmp->timer_step[i] = (uint8_t)step;
    }
    if (!r.ok || r.pos != length)
        return 0;
    tmp->page = 1;
    tmp->focus = 0;
    tmp->stack_count = 0;
    tmp->outside = 0;
    tmp->result_count = 0;
    tmp->dirty = 0;
    *u = *tmp;
    if (sequence)
        *sequence = seq;
    if (clock_lost)
        *clock_lost = lost;
    return 1;
}
