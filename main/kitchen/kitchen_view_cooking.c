#include <stdio.h>
#include <string.h>
#include "kitchen_view_internal.h"
static void fixed(char* out, size_t cap, uint32_t value) {
    if (!(value % 100))
        snprintf(out, cap, "%u", (unsigned)(value / 100));
    else if (!(value % 10))
        snprintf(out, cap, "%u.%u", (unsigned)(value / 100), (unsigned)((value % 100) / 10));
    else
        snprintf(out, cap, "%u.%02u", (unsigned)(value / 100), (unsigned)(value % 100));
}
static void seasoning_quantity(char* out, size_t cap, const char* input, unsigned people,
                               unsigned servings) {
    if (!input || *input < '0' || *input > '9' || !servings) {
        snprintf(out, cap, "%s", input ? input : "");
        return;
    }
    const char* p = input;
    uint64_t number = 0, denominator = 1;
    while (*p >= '0' && *p <= '9') {
        number = number * 10 + (unsigned)(*p - '0');
        ++p;
    }
    if (*p == '.' && p[1] >= '0' && p[1] <= '9') {
        ++p;
        while (*p >= '0' && *p <= '9') {
            number = number * 10 + (unsigned)(*p - '0');
            denominator *= 10;
            ++p;
        }
    }
    uint64_t q = (number * people * 100 + (servings / 2) * denominator) / (servings * denominator);
    if (q > UINT32_MAX) {
        snprintf(out, cap, "%s", input);
        return;
    }
    char n[24];
    fixed(n, sizeof(n), (uint32_t)q);
    snprintf(out, cap, "%s%s", n, p);
}
static void time_text(char* out, size_t cap, int64_t ms) {
    uint64_t sec = ms > 0 ? ((uint64_t)ms + 999) / 1000 : 0;
    snprintf(out, cap, "%02llu:%02llu", (unsigned long long)(sec / 60),
             (unsigned long long)(sec % 60));
}
static void time_ring(kg_canvas* c, int64_t left, int64_t total, int done) {
    uint32_t limit = total > 0 ? (uint32_t)(((uint64_t)left * 65536) / (uint64_t)total) : 0;
    if (limit > 65536)
        limit = 65536;
    /* Restrict work to the active strip; the circle is identical to a full frame. */
    int first_y = c->clip_y0 > 46 ? c->clip_y0 : 46;
    int last_y = c->clip_y1 < 194 ? c->clip_y1 : 194;
    int first_x = c->clip_x0 > 46 ? c->clip_x0 : 46;
    int last_x = c->clip_x1 < 194 ? c->clip_x1 : 194;
    if (first_y < c->y)
        first_y = c->y;
    if (last_y > c->y + c->height)
        last_y = c->y + c->height;
    for (int y = first_y; y < last_y; ++y)
        for (int x = first_x; x < last_x; ++x) {
            int dx = x - 120, dy = y - 120, r = dx * dx + dy * dy;
            if (r < 57 * 57 || r > 71 * 71)
                continue;
            unsigned ax = (unsigned)(dx < 0 ? -dx : dx), ay = (unsigned)(dy < 0 ? -dy : dy), octant,
                     ratio;
            if (dy < 0 && dx >= 0) {
                octant = ax <= ay ? 0 : 1;
                ratio = ax <= ay ? (ax * 8192 / (ay ? ay : 1)) : 8192 - ay * 8192 / ax;
            } else if (dx >= 0 && dy >= 0) {
                octant = ax >= ay ? 2 : 3;
                ratio = ax >= ay ? (ay * 8192 / (ax ? ax : 1)) : 8192 - ax * 8192 / ay;
            } else if (dy >= 0 && dx < 0) {
                octant = ax <= ay ? 4 : 5;
                ratio = ax <= ay ? (ax * 8192 / (ay ? ay : 1)) : 8192 - ay * 8192 / ax;
            } else {
                octant = ax >= ay ? 6 : 7;
                ratio = ax >= ay ? (ay * 8192 / (ax ? ax : 1)) : 8192 - ax * 8192 / ay;
            }
            uint16_t color = octant * 8192 + ratio <= limit ? (done ? RED : BLUE) : PANEL;
            kg_rect(c, x, y, 1, 1, color);
        }
}
static const kt_timer* current_timer(const ku_ui* u, unsigned recipe, unsigned step) {
    for (unsigned i = 0; i < 3; ++i)
        if (u->timer_ids[i] && u->timer_recipe[i] == recipe && u->timer_step[i] == step)
            for (unsigned j = 0; j < u->timers.count; ++j)
                if (u->timers.items[j].id == u->timer_ids[i])
                    return &u->timers.items[j];
    return NULL;
}
static const char* method_label(unsigned i) {
    if (i >= km_methods_count)
        return "做法";
    const char* m = km_methods[i];
    if (!strcmp(m, "cut"))
        return "切";
    if (!strcmp(m, "mix"))
        return "拌";
    if (!strcmp(m, "stir_fry"))
        return "炒";
    if (!strcmp(m, "steam"))
        return "蒸";
    if (!strcmp(m, "boil"))
        return "煮";
    if (!strcmp(m, "braise"))
        return "焖烧";
    if (!strcmp(m, "fry"))
        return "炸";
    if (!strcmp(m, "marinate"))
        return "腌";
    if (!strcmp(m, "blanch"))
        return "焯";
    return "炖";
}
static const char* heat_label(unsigned i) {
    if (i >= km_heats_count)
        return "火候";
    const char* h = km_heats[i];
    return !strcmp(h, "high")  ? "大火"
           : !strcmp(h, "mid") ? "中火"
           : !strcmp(h, "low") ? "小火"
                               : "关火";
}
void kv_cooking_draw(kg_canvas* c, const kv_scene* s) {
    const ku_ui* u = s->state;
    const kv_assets* a = s->assets;
    char text[256];
    if (u->page == 5) {
        kv_header(c, s, "准备食材", YELLOW);
        kg_round_rect(c, 8, 44, 224, 188, 10, PAPER);
        int x0 = c->clip_x0, y0 = c->clip_y0, x1 = c->clip_x1, y1 = c->clip_y1;
        kg_clip(c, 10, 46, 220, 184);
        static const char* units[] = {"克", "个", "根", "瓣", "块", "片", "条",
                                      "只", "把", "颗", "朵", "段", "斤", "适量"};
        for (unsigned i = 0; i < s->ingredient_count; ++i) {
            const kc_ingredient* v = &s->ingredients[i];
            int y = 48 + (int)i * 36 - u->paper_scroll;
            if (v->id >= km_ingredient_count || v->unit >= 14) {
                c->error = 1;
                break;
            }
            /* Do not lay out rows entirely outside the paper viewport: negative
             * offscreen coordinates are valid, not a failed glyph render. */
            if (y + 36 <= 46 || y >= 230)
                continue;
            if (v->main)
                kg_round_rect(c, 12, y + 2, 3, 27, 1, GOLD);
            kv_icon(c, s, km_ingredients[v->id].id, 17, y, 30, 30);
            kv_label(c, s, &a->han16, 52, y + 3, 110, 29, INK, km_ingredients[v->id].name);
            uint64_t amount = ((uint64_t)v->qty100 * u->conditions.people +
                               u->catalog->items[u->recipe].servings / 2) /
                              u->catalog->items[u->recipe].servings;
            char n[24];
            fixed(n, sizeof(n), (uint32_t)amount);
            snprintf(text, sizeof(text), "%s %s", n, units[v->unit]);
            kv_label(c, s, &a->han12, 164, y + 6, 60, 24, INK, text);
            kg_rect(c, 52, y + 33, 167, 1, PAPER2);
        }
        for (unsigned i = 0; i < s->seasoning_count; ++i) {
            const kc_seasoning* v = &s->seasonings[i];
            int y = 48 + (int)s->ingredient_count * 36 + (int)i * 24 - u->paper_scroll;
            if (v->id >= km_seasoning_count) {
                c->error = 1;
                break;
            }
            if (y + 24 <= 46 || y >= 230)
                continue;
            char amount[48];
            seasoning_quantity(amount, sizeof(amount), v->quantity, u->conditions.people,
                               u->catalog->items[u->recipe].servings);
            snprintf(text, sizeof(text), "%s %s", km_seasoning_names[v->id], amount);
            kg_circle(c, 20, y + 9, 2, GOLD);
            kv_label(c, s, &a->han12, 27, y, 195, 22, INK, text);
        }
        c->clip_x0 = x0;
        c->clip_y0 = y0;
        c->clip_x1 = x1;
        c->clip_y1 = y1;
        if (u->paper_scroll_max) {
            unsigned bar = 174u * 180u / (180u + u->paper_scroll_max);
            if (bar < 12)
                bar = 12;
            unsigned top = (174u - bar) * u->paper_scroll / u->paper_scroll_max;
            kg_round_rect(c, 226, 51, 3, 174, 1, PAPER2);
            kg_round_rect(c, 226, 51 + (int)top, 3, (int)bar, 1, GOLD);
        }
        kg_round_rect(c, 8, 242, 224, 38, 9, TOMATO);
        kv_label(c, s, &a->han16, 76, 247, 130, 30, WHITE, "OK 开始跟做");
        kv_footer(c, s, "↓ 加买菜单 · 长 ↓ 翻页 · OK 跟做");
        return;
    }
    if (u->page == 6) {
        kv_header(c, s, u->catalog->items[u->recipe].name, YELLOW);
        for (unsigned i = 0; i < s->step_count; ++i)
            kg_rect(c, 8 + (int)(i * 224 / s->step_count), 46, (int)(224 / s->step_count) - 2, 6,
                    i <= u->sessions[u->recipe].index ? YELLOW : PANEL);
        kg_round_rect(c, 8, 60, 224, 150, 10, PAPER);
        kg_round_rect(c, 18, 69, 31, 27, 8, INK);
        snprintf(text, sizeof(text), "%02u", u->sessions[u->recipe].index + 1);
        kv_label(c, s, &a->latin12, 24, 73, 24, 20, WHITE, text);
        snprintf(text, sizeof(text), "heat_%s", km_heats[s->step.heat]);
        kv_icon(c, s, text, 58, 74, 16, 16);
        kv_label(c, s, &a->han12, 77, 72, 48, 22, INK, heat_label(s->step.heat));
        snprintf(text, sizeof(text), "method_%s", km_methods[s->step.method]);
        kv_icon(c, s, text, 119, 74, 16, 16);
        kv_label(c, s, &a->han12, 139, 72, 49, 22, INK, method_label(s->step.method));
        snprintf(text, sizeof(text), "%u/%u", u->sessions[u->recipe].index + 1, s->step_count);
        kv_label(c, s, &a->latin12, 190, 73, 30, 18, RGB(124, 105, 116), text);
        int x0 = c->clip_x0, y0 = c->clip_y0, x1 = c->clip_x1, y1 = c->clip_y1;
        unsigned height = s->step.tip[0] ? 75 : 100;
        kg_clip(c, 20, 110, 202, height);
        if (kg_text_line(c, a->fonts, &a->han18, 20, 110 - u->step_scroll, 202, 25, INK,
                         s->step.text) < 0)
            c->error = 1;
        c->clip_x0 = x0;
        c->clip_y0 = y0;
        c->clip_x1 = x1;
        c->clip_y1 = y1;
        if (s->step.tip[0]) {
            kg_round_rect(c, 16, 184, 208, 20, 5, YELLOW);
            kv_label(c, s, &a->han12, 21, 185, 198, 19, INK, s->step.tip);
        }
        kg_round_rect(c, 8, 220, 150, 56, 10, PAPER2);
        kg_round_rect(c, 8, 226, 4, 44, 2, s->step.timer_seconds ? BLUE : GREEN);
        const kt_timer* t = current_timer(u, u->recipe, u->sessions[u->recipe].index);
        if (t)
            time_text(text, sizeof(text), kt_remaining(t, s->now));
        else if (s->step.timer_seconds)
            time_text(text, sizeof(text), (int64_t)s->step.timer_seconds * 1000);
        else
            snprintf(text, sizeof(text), "准备好");
        kv_label(c, s, &a->han16, 24, 224, 126, 26, INK, text);
        kv_label(c, s, &a->han12, 24, 251, 126, 20, INK,
                 t && t->status == KT_DONE     ? "时间到"
                 : t && t->status == KT_PAUSED ? "已暂停"
                 : t                           ? "后台计时中"
                 : s->step.timer_seconds       ? "OK 开始计时"
                                               : "OK 下一步");
        if (t && t->total_ms > 0) {
            int64_t left = kt_remaining(t, s->now);
            unsigned progress =
                (unsigned)(((uint64_t)(t->total_ms - left) * 126) / (uint64_t)t->total_ms);
            if (progress > 126)
                progress = 126;
            kg_round_rect(c, 24, 270, 126, 3, 1, RGB(218, 205, 190));
            if (progress)
                kg_round_rect(c, 24, 270, (int)progress, 3, 1, BLUE);
        }
        kv_icon(c, s, s->step.timer_seconds ? "face_focus" : "face_go", 176, 218, 56, 56);
        kv_footer(c, s, "OK 下一步 · ↑ 上一步 · 长 ↓ 用料");
        return;
    }
    if (u->page == 7) {
        kv_header(c, s, "计时", BLUE);
        const kt_timer* t = u->focus < u->timers.count ? &u->timers.items[u->focus] : NULL;
        int64_t left = t ? kt_remaining(t, s->now) : 0;
        time_ring(c, left, t ? t->total_ms : 0, t && t->status == KT_DONE);
        time_text(text, sizeof(text), left);
        kv_label(c, s, &a->latin34, 48, 96, 170, 45, WHITE, text);
        kv_label(c, s, &a->han12, 38, 146, 170, 20, DIM,
                 t ? (t->status == KT_DONE     ? "时间到"
                      : t->status == KT_PAUSED ? "已暂停"
                                               : "后台计时中")
                   : "还没有计时器");
        for (unsigned i = 0; i < u->timers.count; ++i) {
            int y = 202 + (int)i * 27;
            kg_round_rect(c, 8, y, 138, 24, 6, i == u->focus ? GOLD : PANEL);
            time_text(text, sizeof(text), kt_remaining(&u->timers.items[i], s->now));
            kv_label(c, s, &a->han12, 17, y + 2, 120, 21, i == u->focus ? INK : PAPER, text);
        }
        kv_portrait(c, s, 112, 192, 128);
        kv_footer(c, s, "↑↓切换 · 长OK暂停/继续 · 长↓取消");
        return;
    }
    if (u->page == 8) {
        kv_header(c, s, "做好啦！", GREEN);
        kg_circle(c, 120, 108, 60, RGB(43, 46, 57));
        kg_circle(c, 120, 108, 47, RGB(54, 58, 62));
        static const struct {
            int16_t x, y;
            uint16_t color;
        } confetti[] = {{20, 60, GOLD},   {36, 86, BLUE},   {205, 65, TOMATO}, {219, 112, YELLOW},
                        {20, 140, GREEN}, {212, 148, BLUE}, {48, 55, YELLOW},  {192, 132, GOLD}};
        /* Deterministic slow drift requires no retained frame buffer or timer. */
        unsigned phase = (unsigned)((uint64_t)(s->now >= 0 ? s->now : 0) / 240u % 8u);
        for (unsigned i = 0; i < sizeof(confetti) / sizeof(*confetti); ++i) {
            int drift = (int)((phase + i * 3u) % 8u);
            kg_rect(c, confetti[i].x, confetti[i].y + drift, (i & 1) ? 3 : 5, (i & 1) ? 5 : 3,
                    confetti[i].color);
        }
        kv_food(c, s, u->recipe, 40, 48, 160, 120);
        kv_icon(c, s, "ui_stamp_done", 148, 44, 64, 64);
        kv_label(c, s, &a->han20, 12, 176, 220, 30, PAPER, u->catalog->items[u->recipe].name);
        ks_stats stats = ks_get_stats(&u->store, u->date_unconfirmed ? INT32_MIN : u->day);
        snprintf(text, sizeof(text), "我的菜谱 %u / %u", (unsigned)stats.unique, u->catalog->count);
        kv_label(c, s, &a->han12, 12, 208, 120, 20, DIM, text);
        kg_round_rect(c, 12, 230, 113, 5, 2, PANEL);
        unsigned fill = u->catalog->count ? (unsigned)(stats.unique * 113 / u->catalog->count) : 0;
        if (fill > 113)
            fill = 113;
        if (fill)
            kg_round_rect(c, 12, 230, (int)fill, 5, 2, GREEN);
        snprintf(text, sizeof(text), "累计做饭 %u 次", (unsigned)stats.total);
        kv_label(c, s, &a->han12, 12, 241, 120, 20, PAPER, text);
        snprintf(text, sizeof(text), "已解锁 %u / %u", (unsigned)stats.unique, u->catalog->count);
        kv_label(c, s, &a->han12, 12, 262, 120, 20, PAPER, text);
        kv_portrait(c, s, 126, 192, 128);
        kv_footer(c, s, "OK 回吧台");
    }
}
