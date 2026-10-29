#include "kitchen_catalog.h"
#include <string.h>
#include "cJSON.h"
#include "kitchen_metadata.h"

static const cJSON* field(const cJSON* object, const char* key) {
    return cJSON_GetObjectItemCaseSensitive(object, key);
}
static int number(const cJSON* v, int lo, int hi) {
    return cJSON_IsNumber(v) && v->valuedouble == v->valueint && v->valueint >= lo &&
           v->valueint <= hi;
}
static uint32_t hash(const char* s) {
    uint32_t h = 2166136261u;
    while (*s) {
        h ^= (unsigned char)*s++;
        h *= 16777619u;
    }
    return h;
}
static int string_copy(char* out, size_t cap, const cJSON* v) {
    if (!cJSON_IsString(v) || !v->valuestring)
        return 0;
    size_t n = strlen(v->valuestring);
    if (!n || n >= cap)
        return 0;
    memcpy(out, v->valuestring, n + 1);
    return 1;
}
static int load_item(kc_item* out, const krc_pack* pack, unsigned n, const cJSON* dict) {
    const cJSON *ids = field(dict, "ids"), *metadata = field(dict, "metadata"),
                *ingredients = field(dict, "ingredients"), *methods = field(dict, "methods");
    int ingredient_count = cJSON_GetArraySize(ingredients),
        method_count = cJSON_GetArraySize(methods);
    if (!cJSON_IsArray(ids) || !cJSON_IsArray(metadata) || !cJSON_IsArray(ingredients) ||
        !cJSON_IsArray(methods) || cJSON_GetArraySize(ids) != (int)pack->count ||
        cJSON_GetArraySize(metadata) != (int)pack->count || ingredient_count < 1 ||
        ingredient_count > 64 || method_count < 1 || method_count > 64 ||
        pack->bitmap_width != (unsigned)(ingredient_count + 7) / 8)
        return 0;
    krc_index idx;
    if (!krc_index_at(pack, n, &idx))
        return 0;
    const cJSON *meta = cJSON_GetArrayItem(metadata, (int)n), *season = field(meta, "season_mask"),
                *servings = field(meta, "servings");
    if (!cJSON_IsObject(meta) ||
        !string_copy(out->id, sizeof(out->id), cJSON_GetArrayItem(ids, (int)n)) ||
        hash(out->id) != idx.id_hash ||
        !string_copy(out->name, sizeof(out->name), field(meta, "name")) ||
        !number(season, 0, 4095) || !number(servings, 1, 255) || !idx.courses || !idx.minutes ||
        idx.difficulty < 1 || idx.difficulty > 3)
        return 0;
    uint8_t body[KRC_MAX_BODY + 1];
    int len = krc_read_body(pack, n, body, KRC_MAX_BODY);
    if (len < 0)
        return 0;
    body[len] = 0;
    cJSON* json = cJSON_ParseWithLength((const char*)body, (size_t)len + 1);
    if (!json)
        return 0;
    int ok = 0;
    const cJSON *id = field(json, "id"), *list = field(json, "ingredients"),
                *steps = field(json, "steps");
    if (!number(id, (int)n, (int)n) || !cJSON_IsArray(list) || !cJSON_IsArray(steps) ||
        cJSON_GetArraySize(list) < 1 || cJSON_GetArraySize(list) > 10 ||
        cJSON_GetArraySize(steps) < 1 || cJSON_GetArraySize(steps) > 12)
        goto done;
    uint64_t all = 0, main = 0;
    int repeated = 0;
    const cJSON* item;
    cJSON_ArrayForEach (item, list) {
        const cJSON *id_field = field(item, "id"), *is_main = field(item, "main");
        if (!number(id_field, 0, ingredient_count - 1) || (!cJSON_IsBool(is_main) && is_main))
            goto done;
        uint64_t bit = UINT64_C(1) << id_field->valueint;
        all |= bit;
        if (cJSON_IsTrue(is_main)) {
            repeated |= !!(main & bit);
            main |= bit;
        }
    }
    uint64_t packed_main = 0;
    for (unsigned i = 0; i < pack->bitmap_width; ++i)
        packed_main |= (uint64_t)idx.main_bitmap[i] << (8 * i);
    if (!main || main != packed_main)
        goto done;
    unsigned counts[64] = {0};
    cJSON_ArrayForEach (item, steps) {
        const cJSON* method = field(item, "method");
        if (!number(method, 0, method_count - 1))
            goto done;
        const cJSON* label = cJSON_GetArrayItem(methods, method->valueint);
        if (!cJSON_IsString(label) || !label->valuestring)
            goto done;
        if (strcmp(label->valuestring, "cut") && strcmp(label->valuestring, "mix") &&
            strcmp(label->valuestring, "marinate") && strcmp(label->valuestring, "blanch"))
            ++counts[method->valueint];
    }
    int best = -1;
    for (int i = 0; i < method_count; ++i)
        if (counts[i]) {
            const cJSON *a = cJSON_GetArrayItem(methods, i),
                        *b = best < 0 ? NULL : cJSON_GetArrayItem(methods, best);
            if (best < 0 || counts[i] > counts[best] ||
                (counts[i] == counts[best] && strcmp(a->valuestring, b->valuestring) < 0))
                best = i;
        }
    if (best < 0) {
        for (int i = 0; i < method_count; ++i) {
            const cJSON* label = cJSON_GetArrayItem(methods, i);
            if (cJSON_IsString(label) && !strcmp(label->valuestring, "mix")) {
                best = i;
                break;
            }
        }
    }
    if (best < 0)
        goto done;
    out->recipe =
        (kr_recipe){all, main, idx.minutes, 100, idx.courses, (uint8_t)best, (uint8_t)repeated};
    out->hash = idx.id_hash;
    out->season_mask = (uint16_t)season->valueint;
    out->tags = idx.tags;
    out->servings = (uint8_t)servings->valueint;
    out->difficulty = idx.difficulty;
    ok = 1;
done:
    cJSON_Delete(json);
    return ok;
}
int kc_load(kc_catalog* out, const krc_pack* pack) {
    return kc_load_cooperative(out, pack, NULL, NULL);
}
int kc_load_cooperative(kc_catalog* out, const krc_pack* pack,
                        void (*progress)(void*), void* context) {
    if (out)
        out->count = 0;
    if (!out || !out->items || !pack || !pack->data || !pack->count ||
        pack->count > KR_MAX_RECIPES || pack->count > out->capacity)
        return 0;
    const uint8_t* crc = pack->data + 44;
    uint32_t actual =
        (uint32_t)crc[0] | (uint32_t)crc[1] << 8 | (uint32_t)crc[2] << 16 | (uint32_t)crc[3] << 24;
    if (pack->size != km_krc_size || actual != km_krc_crc)
        return 0;
    cJSON* dict = cJSON_ParseWithLength((const char*)pack->data + pack->dictionary_offset,
                                        pack->dictionary_length);
    if (!cJSON_IsObject(dict)) {
        cJSON_Delete(dict);
        return 0;
    }
    for (unsigned i = 0; i < pack->count; ++i) {
        if (!load_item(&out->items[i], pack, i, dict)) {
            cJSON_Delete(dict);
            return 0;
        }
        if (progress)
            progress(context);
    }
    out->count = pack->count;
    cJSON_Delete(dict);
    return 1;
}
int kc_read_step(const krc_pack* pack, unsigned recipe, unsigned step, kc_step* out,
                 unsigned* step_count) {
    if (!pack || !out || recipe >= pack->count)
        return 0;
    uint8_t body[KRC_MAX_BODY + 1];
    int len = krc_read_body(pack, recipe, body, KRC_MAX_BODY);
    if (len < 0)
        return 0;
    body[len] = 0;
    cJSON* json = cJSON_ParseWithLength((const char*)body, (size_t)len + 1);
    if (!json)
        return 0;
    int ok = 0;
    const cJSON* steps = field(json, "steps");
    int count = cJSON_GetArraySize(steps);
    if (!cJSON_IsArray(steps) || count < 1 || count > 12 || step >= (unsigned)count)
        goto done;
    const cJSON *s = cJSON_GetArrayItem(steps, (int)step), *tip = field(s, "tip"),
                *timer = field(s, "timer");
    const cJSON *method = field(s, "method"), *heat = field(s, "heat");
    kc_step temp = {0};
    if (!string_copy(temp.text, sizeof(temp.text), field(s, "text")) ||
        (tip && !string_copy(temp.tip, sizeof(temp.tip), tip)) ||
        (timer && !number(timer, 5, 14400)) || !number(method, 0, (int)km_methods_count - 1) ||
        !number(heat, 0, (int)km_heats_count - 1))
        goto done;
    temp.method = (uint8_t)method->valueint;
    temp.heat = (uint8_t)heat->valueint;
    temp.timer_seconds = timer ? (uint32_t)timer->valueint : 0;
    *out = temp;
    if (step_count)
        *step_count = (unsigned)count;
    ok = 1;
done:
    cJSON_Delete(json);
    return ok;
}
int kc_read_ingredients(const krc_pack* pack, unsigned recipe, kc_ingredient* out,
                        unsigned capacity, unsigned* count) {
    static const char* units[] = {"克", "个", "根", "瓣", "块", "片", "条",
                                  "只", "把", "颗", "朵", "段", "斤", "适量"};
    if (!pack || !out || !count || recipe >= pack->count)
        return 0;
    uint8_t body[KRC_MAX_BODY + 1];
    int len = krc_read_body(pack, recipe, body, KRC_MAX_BODY);
    if (len < 0)
        return 0;
    body[len] = 0;
    cJSON* json = cJSON_ParseWithLength((const char*)body, (size_t)len + 1);
    if (!json)
        return 0;
    int ok = 0;
    const cJSON* list = field(json, "ingredients");
    int n = cJSON_GetArraySize(list);
    if (!cJSON_IsArray(list) || n < 1 || n > 10 || (unsigned)n > capacity)
        goto done;
    for (int i = 0; i < n; ++i) {
        const cJSON *item = cJSON_GetArrayItem(list, i), *id = field(item, "id"),
                    *qty = field(item, "qty"), *unit = field(item, "unit"),
                    *main = field(item, "main");
        if (!number(id, 0, 63) || !cJSON_IsNumber(qty) || qty->valuedouble <= 0 ||
            qty->valuedouble > 1000000 || !cJSON_IsString(unit) || !unit->valuestring ||
            (main && !cJSON_IsBool(main)))
            goto done;
        double scaled = qty->valuedouble * 100.0;
        uint32_t fixed = (uint32_t)(scaled + 0.5);
        if (scaled - (double)fixed > 0.000001 || (double)fixed - scaled > 0.000001)
            goto done;
        unsigned j = 0;
        while (j < sizeof(units) / sizeof(units[0]) && strcmp(unit->valuestring, units[j]))
            ++j;
        if (j == sizeof(units) / sizeof(units[0]))
            goto done;
        out[i] =
            (kc_ingredient){fixed, (uint8_t)id->valueint, (uint8_t)j, (uint8_t)cJSON_IsTrue(main)};
    }
    *count = (unsigned)n;
    ok = 1;
done:
    cJSON_Delete(json);
    return ok;
}
int kc_read_dish(const krc_pack* pack, unsigned recipe, kc_dish* out) {
    if (!pack || !out || recipe >= pack->count)
        return 0;
    uint8_t body[KRC_MAX_BODY + 1];
    int len = krc_read_body(pack, recipe, body, KRC_MAX_BODY);
    if (len < 0)
        return 0;
    body[len] = 0;
    cJSON* json = cJSON_ParseWithLength((const char*)body, (size_t)len + 1);
    if (!json)
        return 0;
    int ok = 0;
    const cJSON *dish = field(json, "dish"), *vessel = field(dish, "vessel"),
                *sauce = field(dish, "sauce");
    const cJSON *layers = field(dish, "layers"), *garnishes = field(dish, "garnish"),
                *hero = field(dish, "hero");
    int nl = cJSON_GetArraySize(layers), ng = garnishes ? cJSON_GetArraySize(garnishes) : 0;
    if (!cJSON_IsObject(dish) || !number(vessel, 0, (int)km_vessels_count - 1) ||
        (!cJSON_IsNull(sauce) && !number(sauce, 0, (int)km_sauces_count - 1)) ||
        !cJSON_IsArray(layers) || nl < 1 || nl > 4 || (garnishes && !cJSON_IsArray(garnishes)) ||
        ng > 3 || (hero && !cJSON_IsBool(hero)))
        goto done;
    kc_dish temp = {0};
    temp.vessel = (uint8_t)vessel->valueint;
    temp.sauce = cJSON_IsNull(sauce) ? -1 : (int16_t)sauce->valueint;
    temp.layer_count = (uint8_t)nl;
    temp.garnish_count = (uint8_t)ng;
    temp.hero_required = cJSON_IsTrue(hero);
    for (int i = 0; i < nl; ++i) {
        const cJSON* x = cJSON_GetArrayItem(layers, i);
        if (!number(x, 0, (int)km_assets_count - 1))
            goto done;
        temp.layers[i] = (uint16_t)x->valueint;
    }
    for (int i = 0; i < ng; ++i) {
        const cJSON* x = cJSON_GetArrayItem(garnishes, i);
        if (!number(x, 0, (int)km_garnishes_count - 1))
            goto done;
        temp.garnishes[i] = (uint16_t)x->valueint;
    }
    *out = temp;
    ok = 1;
done:
    cJSON_Delete(json);
    return ok;
}
int kc_read_seasonings(const krc_pack* pack, unsigned recipe, kc_seasoning* out, unsigned capacity,
                       unsigned* count) {
    if (!pack || !out || !count || recipe >= pack->count)
        return 0;
    uint8_t body[KRC_MAX_BODY + 1];
    int len = krc_read_body(pack, recipe, body, KRC_MAX_BODY);
    if (len < 0)
        return 0;
    body[len] = 0;
    cJSON* json = cJSON_ParseWithLength((const char*)body, (size_t)len + 1);
    if (!json)
        return 0;
    int ok = 0;
    const cJSON* list = field(json, "seasonings");
    int n = cJSON_GetArraySize(list);
    if (!cJSON_IsArray(list) || n < 0 || n > 12 || (unsigned)n > capacity)
        goto done;
    for (int i = 0; i < n; ++i) {
        const cJSON *item = cJSON_GetArrayItem(list, i), *id = field(item, "id");
        if (!number(id, 0, (int)km_seasoning_count - 1) ||
            !string_copy(out[i].quantity, sizeof(out[i].quantity), field(item, "qty")))
            goto done;
        out[i].id = (uint8_t)id->valueint;
    }
    *count = (unsigned)n;
    ok = 1;
done:
    cJSON_Delete(json);
    return ok;
}
