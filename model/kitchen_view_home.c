#include <stdio.h>
#include <string.h>
#include "kitchen_view_internal.h"

static void card(kg_canvas* c, int x, int y, int w, int h, uint16_t fill, int selected) {
    kg_round_rect(c, x, y, w, h, 10, selected ? GOLD : INK);
    kg_round_rect(c, x + 2, y + 2, w - 4, h - 4, 8, fill);
}
static void keyrow(kg_canvas* c, const kv_scene* s, int y, const char* key, const char* text,
                   int hot) {
    card(c, 8, y, 138, 31, hot ? TOMATO : PANEL, 0);
    kg_round_rect(c, 13, y + 4, 30, 22, 5, PAPER2);
    kv_label(c, s, &s->assets->han12, 17, y + 4, 24, 20, INK, key);
    kv_label(c, s, &s->assets->han15, 49, y + 3, 94, 25, PAPER, text);
}
/* Avoid decompressing every static sprite for stripes it cannot touch. */
static int boot_ui_sprite(kg_canvas* c, const kv_scene* s, const char* name, int x, int y,
                          unsigned width, unsigned height) {
    if (y + (int)height <= c->y || y >= c->y + c->height)
        return 1;
    ksp_sprite sprite;
    return ksp_find(s->assets->ui, name, &sprite) &&
           kg_sprite_scaled(c, s->assets->ui, &sprite, x, y, width, height);
}
void kv_home_draw(kg_canvas* c, const kv_scene* s) {
    const ku_ui* u = s->state;
    char text[96];
    if (u->page == 0) {
        int64_t remaining = u->transition_due > s->now ? u->transition_due - s->now : 0;
        int elapsed = remaining >= KU_BOOT_DURATION_MS ? 0 : KU_BOOT_DURATION_MS - (int)remaining;
        static const uint8_t stars[][2] = {{8, 35},   {29, 33},  {54, 34},   {79, 30},
                                           {162, 29}, {191, 34}, {213, 31},  {27, 68},
                                           {12, 112}, {55, 125}, {184, 111}, {218, 124}};
        kg_rect(c, 0, 0, 240, 320, BG);
        for (unsigned i = 0; i < sizeof(stars) / sizeof(*stars); ++i) {
            int bright = ((elapsed / 650 + (int)i) % 3) == 0;
            kg_rect(c, stars[i][0], stars[i][1], bright ? 2 : 1, bright ? 2 : 1,
                    bright ? PAPER2 : DIM);
        }
        /* Crescent and lanterns are independent of the character animation frames. */
        kg_circle(c, 209, 54, 12, GOLD);
        kg_circle(c, 214, 50, 10, BG);
        if (!boot_ui_sprite(c, s, "ui_lantern", 8, 149, 27, 27) ||
            !boot_ui_sprite(c, s, "ui_lantern", 205, 149, 27, 27) ||
            !boot_ui_sprite(c, s, "logo_wordmark", 10, 42, 220, 90))
            c->error = 1;
        kv_label(c, s, &s->assets->han12, 78, 138, 140, 22, DIM, "那波，陪你做饭");
        static const struct {
            const char* name;
            int x, y;
        } food[] = {{"tomato", 39, 185}, {"choy_sum", 81, 168}, {"fish_whole", 133, 170},
                    {"tofu", 179, 187},  {"egg", 17, 220},      {"pork_belly", 200, 227}};
        for (unsigned i = 0; i < sizeof(food) / sizeof(*food); ++i) {
            int progress = elapsed - 450 - (int)i * 180;
            if (progress <= 0)
                continue;
            if (progress > 280)
                progress = 280;
            unsigned size = (unsigned)(28 * progress / 280);
            if (!size)
                size = 1;
            int x = food[i].x + (28 - (int)size) / 2;
            int y = food[i].y + (280 - progress) * 18 / 280 + (28 - (int)size) / 2;
            kv_icon(c, s, food[i].name, x, y, size, size);
        }
        int rise = elapsed - 1450;
        if (rise > 0) {
            if (rise > 800)
                rise = 800;
            int y = 164 + (800 - rise) * 156 / 800;
            kv_portrait(c, s, 42, y, 156);
        }
        return;
    }
    if (u->page == 1) {
        kv_header(c, s, "NABO 深夜厨房", GOLD);
        kg_rect(c, 0, 38, 240, 70, PANEL);
        kv_icon(c, s, "face_happy", 8, 44, 58, 58);
        card(c, 72, 46, 158, 54, PAPER, 0);
        kv_label(c, s, &s->assets->han16, 82, 48, 142, 25, INK, "今晚吃点啥？");
        unsigned left = u->reroll_limit > u->rerolls_used ? u->reroll_limit - u->rerolls_used : 0;
        snprintf(text, sizeof(text), "%s · 剩 %u 次",
                 u->has_saved ? "今天已经抽签" : "今天还没抽签", left);
        kv_label(c, s, &s->assets->han12, 82, 74, 142, 22, INK, text);
        card(c, 8, 114, 224, 58, TOMATO, u->focus == 0);
        kv_icon(c, s, "method_stir_fry", 16, 125, 36, 36);
        kv_label(c, s, &s->assets->han18, 60, 116, 164, 27, INK, "今天吃什么");
        kv_label(c, s, &s->assets->han12, 60, 144, 164, 20, INK, "颠一下锅，马上决定");
        const char* names[] = {"冰箱", "买菜单", "我的菜谱", "设置"};
        const char* icons[] = {"ui_fridge", "ui_shopping", "ui_recipe_book", "ui_settings"};
        uint16_t colors[] = {BLUE, GREEN, YELLOW, RGB(197, 195, 211)};
        for (unsigned i = 0; i < 4; ++i) {
            int x = 8 + (i % 2) * 116, y = 180 + (i / 2) * 50;
            card(c, x, y, 108, 44, PAPER, u->focus == i + 1);
            kg_rect(c, x + 3, y + 4, 4, 36, colors[i]);
            kv_icon(c, s, icons[i], x + 10, y + 10, 24, 24);
            kv_label(c, s, &s->assets->han15, x + 37, y + 2, 68, 22, INK, names[i]);
            if (i == 0)
                snprintf(text, sizeof(text), "有啥做啥");
            else if (i == 1) {
                unsigned pending = 0;
                for (unsigned j = 0; j < u->shopping_count; ++j)
                    pending += !u->shopping[j].bought;
                snprintf(text, sizeof(text), "%u 样待买", pending);
            } else if (i == 2) {
                ks_stats stats = ks_get_stats(&u->store, u->day);
                snprintf(text, sizeof(text), "%u / %u", (unsigned)stats.unique, u->catalog->count);
            } else
                snprintf(text, sizeof(text), "口味 · 声音");
            kv_label(c, s, &s->assets->han12, x + 37, y + 23, 68, 18, INK, text);
        }
        kv_footer(c, s, "↑↓ 选择 · OK 进入 · 长 OK 颠勺");
        return;
    }
    if (u->page == 2) {
        kv_header(c, s, "今晚怎么吃", TOMATO);
        const char* names[] = {"吃几个人", "有多少时间", "怎么吃"};
        const char* combos[] = {"一道菜", "一荤一素", "一荤一素一汤", "两荤一素一汤"};
        for (unsigned i = 0; i < 3; ++i) {
            int y = 52 + i * 52;
            card(c, 8, y, 224, 44, PAPER, u->focus == i);
            kv_label(c, s, &s->assets->han16, 20, y + 8, 105, 30, INK, names[i]);
            if (i == 0)
                snprintf(text, sizeof(text), "%u 人", u->conditions.people);
            else if (i == 1) {
                if (u->conditions.minutes)
                    snprintf(text, sizeof(text), "%u 分钟", u->conditions.minutes);
                else
                    snprintf(text, sizeof(text), "不限");
            } else
                snprintf(text, sizeof(text), "%s", combos[u->conditions.combo]);
            kv_label(c, s, &s->assets->han15, 122, y + 8, 105, 30, INK, text);
        }
        card(c, 8, 224, 224, 42, TOMATO, u->focus == 3);
        kv_label(c, s, &s->assets->han18, 80, 229, 100, 32, WHITE, "颠勺！");
        kv_footer(c, s, "↑↓ 选行 · OK 切换 · 颠勺出菜");
        return;
    }
    if (u->page == 3) {
        kv_header(c, s, "今晚吃点啥", TOMATO);
        kv_label(c, s, &s->assets->han16, 44, 60, 180, 30, PAPER, "今晚吃点热乎的");
        kv_portrait(c, s, 24, 92, 192);
        return;
    }
    if (u->page == 4) {
        kv_header(c, s, "今天就吃它！", TOMATO);
        if (u->result_count > 1) {
            for (unsigned i = 0; i < u->result_count && i < 3; ++i) {
                unsigned start = u->result_focus > 0 ? u->result_focus - 1 : 0;
                if (start + 3 > u->result_count)
                    start = u->result_count > 3 ? u->result_count - 3 : 0;
                unsigned n = start + i;
                if (n >= u->result_count)
                    break;
                int x = 8 + i * 76, y = n == u->result_focus ? 66 : 74;
                card(c, x, y, 72, 104, n == u->result_focus ? PAPER : PANEL, n == u->result_focus);
                kv_food(c, s, u->result[n], x, y + 2, 72, 54);
                kv_label(c, s, &s->assets->han12, x + 2, y + 58, 68, 42,
                         n == u->result_focus ? INK : PAPER, u->catalog->items[u->result[n]].name);
            }
            kv_icon(c, s, "face_happy", 10, 185, 40, 40);
            card(c, 60, 186, 170, 52, PAPER, 0);
            kv_label(c, s, &s->assets->han12, 66, 192, 158, 40, INK, "先准备食材，再开始做菜");
            keyrow(c, s, 246, "↓", "换一组", 0);
            keyrow(c, s, 280, "OK", "开始做这道", 1);
        } else if (u->result_count) {
            card(c, 8, 44, 224, 116, PANEL, 0);
            /* A warm little stage keeps every layered dish visually distinct. */
            kg_circle(c, 120, 101, 53, RGB(58, 48, 75));
            kg_circle(c, 120, 101, 42, RGB(71, 53, 75));
            kg_rect(c, 22, 147, 196, 2, RGB(84, 62, 77));
            kg_rect(c, 30, 76, 2, 8, GOLD);
            kg_rect(c, 27, 79, 8, 2, GOLD);
            kg_rect(c, 205, 100, 2, 7, RGB(124, 100, 125));
            kg_rect(c, 203, 102, 6, 2, RGB(124, 100, 125));
            kv_food(c, s, u->recipe, 45, 46, 150, 112);
            kv_label(c, s, &s->assets->han20, 12, 164, 220, 30, PAPER,
                     u->catalog->items[u->recipe].name);
            kg_round_rect(c, 12, 192, 75, 20, 6, PANEL);
            kv_icon(c, s, "ui_timer_icon", 17, 194, 14, 14);
            snprintf(text, sizeof(text), "%u 分钟", u->catalog->items[u->recipe].recipe.minutes);
            kv_label(c, s, &s->assets->han12, 34, 193, 53, 18, PAPER, text);
            kg_round_rect(c, 91, 192, 53, 20, 6, PANEL);
            snprintf(text, sizeof(text), "难度 %u", u->catalog->items[u->recipe].difficulty);
            kv_label(c, s, &s->assets->han12, 95, 193, 48, 18, PAPER, text);
            keyrow(c, s, 216, "↑", "回吧台", 0);
            keyrow(c, s, 250, "↓", "换一道", 0);
            keyrow(c, s, 284, "OK", "开做！", 1);
            kv_portrait(c, s, 126, 192, 128);
        }
        if (u->notes) {
            char note[90] = "已放宽: ";
            if (u->notes & KR_MAIN_RELAXED)
                snprintf(note + strlen(note), sizeof(note) - strlen(note), "主料重复 ");
            if (u->notes & KR_METHOD_RELAXED)
                snprintf(note + strlen(note), sizeof(note) - strlen(note), "做法相同 ");
            if (u->notes & KR_TIME_RELAXED)
                snprintf(note + strlen(note), sizeof(note) - strlen(note), "总用时");
            kv_label(c, s, &s->assets->han12, 12, 47, 220, 22, GOLD, note);
        }
    }
}
