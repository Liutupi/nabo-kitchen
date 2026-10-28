#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "kitchen_view_internal.h"
static int font(const kfn_pack* p, const char* name, kfn_font* out) {
    return kfn_find_font(p, name, out);
}
int kv_assets_init(kv_assets* out, const kc_catalog* catalog, const krc_pack* recipes,
                   const kfn_pack* fonts, const ksp_pack* dish, const ksp_pack* icons,
                   const ksp_pack* ui, const kan_pack* animation) {
    if (!out || !catalog || !recipes || !fonts || !dish || !icons || !ui || !animation ||
        catalog->count > KR_MAX_RECIPES)
        return 0;
    memset(out, 0, sizeof(*out));
    out->fonts = fonts;
    out->dish = dish;
    out->icons = icons;
    out->ui = ui;
    out->animation = animation;
    if (!font(fonts, "han12", &out->han12) || !font(fonts, "han15", &out->han15) ||
        !font(fonts, "han16", &out->han16) || !font(fonts, "han18", &out->han18) ||
        !font(fonts, "han20", &out->han20) || !font(fonts, "latin12", &out->latin12) ||
        !font(fonts, "latin34", &out->latin34))
        return 0;
    for (unsigned i = 0; i < catalog->count; ++i)
        if (!kc_read_dish(recipes, i, &out->dishes[i]))
            return 0;
    out->dish_count = catalog->count;
    return 1;
}
int kv_prepare(kv_scene* scene, ku_ui* u, const kv_assets* a, int64_t now, int battery,
               const char* clock, const uint8_t* frame) {
    if (!scene || !u || !a || !u->catalog || u->recipe >= u->catalog->count ||
        a->dish_count != u->catalog->count)
        return 0;
    memset(scene, 0, sizeof(*scene));
    scene->state = u;
    scene->assets = a;
    scene->now = now;
    scene->battery = battery;
    scene->animation_frame = frame;
    snprintf(scene->clock, sizeof(scene->clock), "%s",
             clock && strlen(clock) == 5 ? clock : "--:--");
    if (u->page == 5 &&
        (!kc_read_ingredients(u->pack, u->recipe, scene->ingredients, 10,
                              &scene->ingredient_count) ||
         !kc_read_seasonings(u->pack, u->recipe, scene->seasonings, 12, &scene->seasoning_count)))
        return 0;
    if (u->page == 6) {
        if (!kc_read_step(u->pack, u->recipe, u->sessions[u->recipe].index, &scene->step,
                          &scene->step_count))
            return 0;
        uint16_t dummy[240];
        kg_canvas c;
        kg_init(&c, dummy, 240, 0, 1, 0);
        kg_clip(&c, 0, 0, 0, 0);
        int bottom = kg_text_line(&c, a->fonts, &a->han18, 20, 110, 202, 25, INK, scene->step.text);
        if (bottom < 0)
            return 0;
        unsigned area = scene->step.tip[0] ? 75 : 100;
        ku_scroll_extents(u, bottom > 110 + (int)area ? (unsigned)(bottom - 110 - (int)area) : 0,
                          u->paper_scroll_max);
    }
    if (u->page == 5) {
        unsigned height = scene->ingredient_count * 36 + scene->seasoning_count * 24;
        ku_scroll_extents(u, u->step_scroll_max, height > 180 ? height - 180 : 0);
    }
    if (u->page == 9 || u->page == 10) {
        scene->match_count = ku_fridge_matches(u, scene->fridge_matches, KR_MAX_RECIPES);
        if (scene->match_count == UINT_MAX)
            return 0;
        for (unsigned i = 0; i < scene->match_count; ++i) {
            unsigned recipe = scene->fridge_matches[i];
            scene->fridge_complete_count +=
                !(u->catalog->items[recipe].recipe.main_ingredients & ~u->fridge);
        }
    }
    if (u->page == 12) {
        scene->book_count = ku_book_list(u, scene->book, KR_MAX_RECIPES);
        if (scene->book_count == UINT_MAX)
            return 0;
    }
    if (u->page == 11) {
        scene->shopping_count = ku_shopping_order(u, scene->shopping, KU_SHOPPING_CAPACITY);
        if (scene->shopping_count == UINT_MAX)
            return 0;
    }
    return 1;
}
void kv_label(kg_canvas* c, const kv_scene* s, const kfn_font* f, int x, int y, int w, int h,
              uint16_t color, const char* text) {
    int x0 = c->clip_x0, y0 = c->clip_y0, x1 = c->clip_x1, y1 = c->clip_y1;
    kg_clip(c, x, y, w, h);
    if (kg_text_line(c, s->assets->fonts, f, x, y, w, f == &s->assets->han18 ? 25 : f->line_height,
                     color, text) < 0)
        c->error = 1;
    c->clip_x0 = x0;
    c->clip_y0 = y0;
    c->clip_x1 = x1;
    c->clip_y1 = y1;
}
void kv_icon(kg_canvas* c, const kv_scene* s, const char* id, int x, int y, unsigned w,
             unsigned h) {
    if (y + (int)h <= c->y || y >= c->y + c->height)
        return;
    ksp_sprite sprite;
    if (!ksp_find(s->assets->icons, id, &sprite) ||
        !kg_sprite_scaled(c, s->assets->icons, &sprite, x, y, w, h))
        c->error = 1;
}
static void layer(kg_canvas* c, const kv_scene* s, const char* id, int x, int y, unsigned w,
                  unsigned h) {
    if (y + (int)h <= c->y || y >= c->y + c->height)
        return;
    ksp_sprite sprite;
    if (!ksp_find(s->assets->dish, id, &sprite) ||
        !kg_sprite_scaled(c, s->assets->dish, &sprite, x, y, w, h))
        c->error = 1;
}
void kv_food(kg_canvas* c, const kv_scene* s, unsigned recipe, int x, int y, unsigned w,
             unsigned h) {
    if (recipe >= s->assets->dish_count) {
        c->error = 1;
        return;
    }
    /* Full illustrations are optional; keep layered dishes as the fallback. */
    if (y + (int)h <= c->y || y >= c->y + c->height)
        return;
    char hero_id[48];
    snprintf(hero_id, sizeof(hero_id), "hero_%s", s->state->catalog->items[recipe].id);
    ksp_sprite hero;
    if (ksp_find(s->assets->dish, hero_id, &hero)) {
        if (hero.frame_width != 160 || hero.frame_height != 120 ||
            !kg_sprite_scaled(c, s->assets->dish, &hero, x, y, w, h))
            c->error = 1;
        return;
    }
    const kc_dish* d = &s->assets->dishes[recipe];
    /* Two original recipes historically marked hero=true as a future art
     * request and already shipped using layers; keep their established view.
     * New recipes requesting a hero must not depict a misleading substitute. */
    const char* recipe_id = s->state->catalog->items[recipe].id;
    if (d->hero_required && strcmp(recipe_id, "fanqie-chaodan") &&
        strcmp(recipe_id, "qingzheng-luyu")) {
        kg_round_rect(c, x, y, (int)w, (int)h, 7, PANEL);
        const char* text = w >= 70 ? "菜图制作中" : "待配图";
        int text_width = w >= 70 ? 60 : 36;
        kv_label(c, s, &s->assets->han12, x + ((int)w - text_width) / 2,
                 y + ((int)h - 18) / 2, text_width, 18, PAPER, text);
        return;
    }
    const char* vessel = km_vessels[d->vessel];
    char name[140];
    snprintf(name, sizeof(name), "vessel_%s_shadow", vessel);
    layer(c, s, name, x, y, w, h);
    snprintf(name, sizeof(name), "vessel_%s", vessel);
    layer(c, s, name, x, y, w, h);
    if (d->sauce >= 0) {
        const char* group =
            !strcmp(vessel, "bowl") || !strcmp(vessel, "claypot") ? vessel : "plate";
        snprintf(name, sizeof(name), "sauce_%s__%s", km_sauces[d->sauce], group);
        layer(c, s, name, x, y, w, h);
    }
    for (unsigned i = 0; i < d->layer_count; ++i)
        layer(c, s, km_assets[d->layers[i]], x, y, w, h);
    for (unsigned i = 0; i < d->garnish_count; ++i) {
        snprintf(name, sizeof(name), "garnish_%s", km_garnishes[d->garnishes[i]]);
        layer(c, s, name, x, y, w, h);
    }
}
void kv_portrait(kg_canvas* c, const kv_scene* s, int x, int y, unsigned size) {
    if (!s->animation_frame || y + (int)size <= c->y || y >= c->y + c->height)
        return;
    const kan_pack* a = s->assets->animation;
    kg_indexed_scaled(c, x, y, size, size, s->animation_frame, a->width, a->height, a->data + 52,
                      a->palette_count);
}
void kv_header(kg_canvas* c, const kv_scene* s, const char* title, uint16_t accent) {
    kg_rect(c, 0, 0, 240, 32, INK);
    kv_label(c, s, &s->assets->han16, 8, 2, 162, 28, PAPER, title);
    kv_label(c, s, &s->assets->latin12, 184, 6, 50, 20, GOLD, s->clock);
    if (s->battery >= 0) {
        char b[16];
        snprintf(b, sizeof(b), "%d%%", s->battery > 100 ? 100 : s->battery);
        kv_label(c, s, &s->assets->latin12, 149, 6, 34, 20, DIM, b);
    }
    kg_rect(c, 0, 32, 240, 6, accent);
    for (int y = 0; y < 2; ++y)
        for (int x = y % 2; x < 80; x += 2)
            kg_rect(c, x * 3, 32 + y * 3, 3, 3, PAPER2);
}
void kv_footer(kg_canvas* c, const kv_scene* s, const char* text) {
    kg_rect(c, 0, 292, 240, 28, INK);
    kv_label(c, s, &s->assets->han12, 7, 294, 226, 24, PAPER, text);
}
int kv_draw(kg_canvas* c, const kv_scene* s) {
    if (!c || !s || !s->state || !s->assets)
        return 0;
    unsigned p = s->state->page;
    if (p <= 4)
        kv_home_draw(c, s);
    else if (p <= 8)
        kv_cooking_draw(c, s);
    else if (p <= 13)
        kv_lists_draw(c, s);
    else
        return 0;
    for (unsigned i = 0; i < s->state->timers.count; ++i) {
        const kt_timer* t = &s->state->timers.items[i];
        if (t->status == KT_DONE && !t->acknowledged) {
            /* Startup and home reserve their top edge for a timer alert so it
             * never covers NABO, the menu cards or the date hint. */
            if (p == 0) {
                kg_round_rect(c, 38, 3, 164, 24, 6, RED);
                kv_label(c, s, &s->assets->han12, 53, 5, 145, 20, WHITE,
                         "时间到 · OK 查看");
            } else if (p == 1) {
                kg_rect(c, 0, 0, 240, 32, RED);
                kv_label(c, s, &s->assets->han12, 66, 6, 160, 22, WHITE,
                         "时间到 · OK 查看");
            } else {
                kg_round_rect(c, 8, 267, 224, 24, 6, RED);
                kv_label(c, s, &s->assets->han12, 20, 269, 204, 20, WHITE,
                         "时间到 · OK 查看");
            }
            if (s->now >= t->deadline_ms && s->now - t->deadline_ms < 2100 &&
                ((s->now - t->deadline_ms) / 350) % 2 == 0) {
                kg_rect(c, 0, 0, 240, 3, RED);
                kg_rect(c, 0, 317, 240, 3, RED);
                kg_rect(c, 0, 0, 3, 320, RED);
                kg_rect(c, 237, 0, 3, 320, RED);
            }
            break;
        }
    }
    if (s->state->message) {
        static const char* messages[] = {"",
                                         "没有符合忌口的菜谱",
                                         "现有菜谱无法组成搭配",
                                         "今天的重抽次数用完啦",
                                         "已有 3 个计时，请先停止一个",
                                         "还有步骤未完成",
                                         "菜谱加载失败",
                                         "空间不足，请重试",
                                         "本地保存失败，请检查存储",
                                         "时钟不可用，计时已暂停",
                                         "已恢复较早的记录"};
        unsigned code = s->state->message;
        if (code < sizeof(messages) / sizeof(messages[0])) {
            kg_round_rect(c, 20, 130, 200, 48, 8, INK);
            kv_label(c, s, &s->assets->han15, 28, 138, 184, 35, PAPER, messages[code]);
        }
    }
    return !c->error;
}
