#include <stdio.h>
#include <string.h>
#include "kitchen_view_internal.h"
static void fixed(char* out, size_t cap, uint32_t q) {
    if (!(q % 100))
        snprintf(out, cap, "%u", (unsigned)(q / 100));
    else if (!(q % 10))
        snprintf(out, cap, "%u.%u", (unsigned)(q / 100), (unsigned)(q % 100 / 10));
    else
        snprintf(out, cap, "%u.%02u", (unsigned)(q / 100), (unsigned)(q % 100));
}
static int cooked(const ku_ui* u, unsigned recipe) {
    for (unsigned i = 0; i < u->store.cooked_count; ++i)
        if (u->store.cooked[i].recipe == recipe)
            return 1;
    return 0;
}
void kv_lists_draw(kg_canvas* c, const kv_scene* s) {
    const ku_ui* u = s->state;
    const kv_assets* a = s->assets;
    char text[120];
    if (u->page == 9) {
        kv_header(c, s, "冰箱里有啥", BLUE);
        static const char* names[KU_FRIDGE_CATEGORIES] = {"肉", "海鲜", "蛋豆", "蔬菜", "主食"};
        unsigned selected = u->fridge_category < KU_FRIDGE_CATEGORIES ? u->fridge_category : 0;
        for (unsigned i = 0; i < KU_FRIDGE_CATEGORIES; ++i) {
            int x = 6 + (int)i * 46;
            int active = selected == i;
            kg_round_rect(c, x, 44, 43, 22, 7, active && !u->fridge_selecting_items ? GOLD :
                          active ? BLUE : PANEL);
            kv_label(c, s, &a->han12, x + (i ? 8 : 15), 45, 36, 20, active ? INK : PAPER,
                     names[i]);
        }
        uint8_t ordered[64];
        unsigned n = ku_fridge_category_items(u, selected, ordered, 64), pos = 0;
        if (u->fridge_selecting_items)
            while (pos < n && ordered[pos] != u->focus)
                ++pos;
        if (pos >= n)
            pos = 0;
        unsigned start = pos / 8 * 8;
        for (unsigned j = 0; j < 8 && start + j < n; ++j) {
            unsigned ordinal = ordered[start + j], id = u->fridge_order[ordinal];
            int x = 8 + (int)(j % 4) * 58, y = 74 + (int)(j / 4) * 72;
            kg_round_rect(c, x, y, 52, 66, 8,
                          u->fridge_selecting_items && ordinal == u->focus ? GOLD
                          : (u->fridge & (UINT64_C(1) << id)) ? GREEN
                                                              : PAPER2);
            kv_icon(c, s, km_ingredients[id].id, x + 6, y + 3, 40, 40);
            /* Selected and focused are separate states: the gold focus must not
             * hide an existing fridge selection. Draw a font-independent tick. */
            if (u->fridge & (UINT64_C(1) << id)) {
                kg_circle(c, x + 45, y + 9, 6, GREEN);
                kg_rect(c, x + 41, y + 9, 2, 2, WHITE);
                kg_rect(c, x + 43, y + 11, 2, 2, WHITE);
                kg_rect(c, x + 45, y + 9, 2, 2, WHITE);
                kg_rect(c, x + 47, y + 7, 2, 2, WHITE);
            }
            /* The full metadata name is useful elsewhere but does not fit a
             * 48 px refrigerator tile. Keep the stable ingredient ID intact. */
            const char* name = !strcmp(km_ingredients[id].id, "fish_whole")
                                   ? "鱼"
                                   : km_ingredients[id].name;
            kv_label(c, s, &a->han12, x + 2, y + 44, 48, 20, INK, name);
        }
        if (!n)
            kv_label(c, s, &a->han15, 50, 110, 150, 30, PAPER, "此分类暂无食材");
        kv_icon(c, s, "face_focus", 10, 224, 40, 40);
        kg_round_rect(c, 60, 226, 170, 52, 10, PAPER);
        if (u->fridge)
            snprintf(text, sizeof(text), "这些能做 %u 道菜", s->fridge_complete_count);
        else
            snprintf(text, sizeof(text), "先选择冰箱食材");
        kv_label(c, s, &a->han12, 68, 230, 154, 20, INK, text);
        kv_label(c, s, &a->han12, 68, 251, 154, 19, INK, "长按 OK 看推荐");
        kv_footer(c, s, u->fridge_selecting_items ? "↑↓ 选食材 · OK 勾选 · 双 ↑ 分类"
                                                 : "↑↓ 选分类 · OK 进入 · 双 ↑ 返回");
        return;
    }
    if (u->page == 10) {
        kv_header(c, s, "冰箱推荐", BLUE);
        unsigned start = (u->focus / 4) * 4;
        if (!s->match_count)
            kv_label(c, s, &a->han16, 15, 96, 210, 65, PAPER,
                     u->fridge ? "暂无符合的菜" : "先选择冰箱食材");
        for (unsigned i = 0; i < 4 && start + i < s->match_count; ++i) {
            unsigned recipe = s->fridge_matches[start + i];
            int y = 46 + (int)i * 57;
            kg_round_rect(c, 8, y, 224, 52, 9, start + i == u->focus ? GOLD : PANEL);
            kv_food(c, s, recipe, 14, y + 8, 48, 36);
            kv_label(c, s, &a->han15, 68, y + 2, 155, 23, start + i == u->focus ? INK : PAPER,
                     u->catalog->items[recipe].name);
            uint64_t missing = u->catalog->items[recipe].recipe.main_ingredients & ~u->fridge;
            if (missing) {
                unsigned id = 0;
                while (id < 64 && !(missing & (UINT64_C(1) << id)))
                    ++id;
                snprintf(text, sizeof(text), "缺 1 样: %s",
                         id < km_ingredient_count ? km_ingredients[id].name : "");
            } else
                snprintf(text, sizeof(text), "齐了");
            kv_label(c, s, &a->han12, 68, y + 26, 150, 20, start + i == u->focus ? INK : DIM, text);
        }
        kv_footer(c, s, "↑↓ 选择 · OK 看菜谱");
        return;
    }
    if (u->page == 11) {
        kv_header(c, s, "买菜单", GREEN);
        kg_round_rect(c, 14, 50, 212, 232, 8, PAPER);
        kg_rect(c, 86, 45, 68, 12, YELLOW);
        unsigned start = (u->focus / 5) * 5;
        static const char* units[] = {"克", "个", "根", "瓣", "块", "片", "条",
                                      "只", "把", "颗", "朵", "段", "斤", "适量"};
        for (unsigned i = 0; i < 5 && start + i < s->shopping_count; ++i) {
            const ku_shopping* item = &u->shopping[s->shopping[start + i]];
            int y = 60 + (int)i * 37;
            if (start + i == u->focus)
                kg_round_rect(c, 19, y, 202, 35, 5, GOLD);
            kg_rect(c, 23, y + 10, 12, 12, item->bought ? GREEN : INK);
            kv_icon(c, s, km_ingredients[item->id].id, 38, y + 2, 30, 30);
            kv_label(c, s, &a->han12, 70, y + 5, 75, 23, INK, km_ingredients[item->id].name);
            char n[24];
            fixed(n, sizeof(n), item->qty100);
            snprintf(text, sizeof(text), "%s %s", n, item->unit < 14 ? units[item->unit] : "");
            kv_label(c, s, &a->han12, 145, y + 5, 72, 23, INK, text);
        }
        if (!s->shopping_count)
            kv_label(c, s, &a->han16, 40, 104, 180, 70, INK, "买菜单是空的\n从用料页加入食材");
        if (u->focus == s->shopping_count) kg_round_rect(c, 22, 249, 196, 24, 6, GOLD);
        kv_label(c, s, &a->han12, 76, 251, 120, 20, INK, "清空已买");
        kv_footer(c, s, "↑↓ 选择 · OK 打勾 / 清空");
        return;
    }
    if (u->page == 12) {
        kv_header(c, s, "我的菜谱", YELLOW);
        ks_stats stats = ks_get_stats(&u->store, u->date_unconfirmed ? INT32_MIN : u->day);
        kg_round_rect(c, 8, 44, 224, 52, 9, PAPER2);
        snprintf(text, sizeof(text), "badge_%u", (unsigned)stats.level);
        kv_icon(c, s, text, 14, 50, 40, 40);
        static const char* levels[] = {"", "夜宵学徒", "深夜帮厨", "夜班主厨", "深夜厨神"};
        kv_label(c, s, &a->han16, 62, 46, 160, 26, INK, levels[stats.level]);
        snprintf(text, sizeof(text), "累计 %u 次 · 解锁 %u 道", (unsigned)stats.total, (unsigned)stats.unique);
        kv_label(c, s, &a->han12, 62, 73, 160, 20, INK, text);
        static const char* filters[] = {"全部", "做过的", "收藏", "我存的"};
        snprintf(text, sizeof(text), "筛选: %s", filters[u->book_filter]);
        if (!u->focus) kg_round_rect(c, 8, 99, 224, 23, 5, GOLD);
        kv_label(c, s, &a->han12, 10, 100, 220, 22, u->focus ? PAPER : INK, text);
        unsigned start = u->focus ? ((u->focus - 1) / 6) * 6 : 0;
        for (unsigned i = 0; i < 6 && start + i < s->book_count; ++i) {
            unsigned recipe = s->book[start + i];
            int x = 8 + (int)(i % 3) * 76, y = 125 + (int)(i / 3) * 83;
            int selected = u->focus == start + i + 1;
            kg_round_rect(c, x, y, 72, 80, 8, selected ? GOLD : PANEL);
            if (cooked(u, recipe))
                kv_food(c, s, recipe, x, y + 1, 72, 54);
            else
                kv_label(c, s, &a->han20, x + 28, y + 12, 30, 34, PAPER, "?");
            kv_label(c, s, &a->han12, x + 3, y + 56, 66, 22, selected ? INK : PAPER,
                     cooked(u, recipe) ? u->catalog->items[recipe].name : "还没做过");
        }
        kv_footer(c, s, "↑↓ 选择 · OK 打开 · 长 OK 收藏");
        return;
    }
    if (u->page == 13) {
        kv_header(c, s, "设置", RGB(197, 195, 211));
        unsigned start = (u->focus / 5) * 5;
        static const char* names[] = {"声音", "音量", "不吃的食材", "少辣",
                                      "少油", "难度", "清空记录"};
        unsigned total = u->settings_mode ? u->ingredient_count : 7;
        for (unsigned i = 0; i < 5 && start + i < total; ++i) {
            unsigned index = start + i;
            int y = 46 + (int)i * 46;
            kg_round_rect(c, 8, y, 224, 42, 8, index == u->focus ? GOLD : PAPER2);
            if (u->settings_mode) {
                unsigned id = u->ingredient_order[index];
                kv_icon(c, s, km_ingredients[id].id, 14, y + 5, 32, 32);
                kv_label(c, s, &a->han15, 52, y + 6, 110, 28, INK, km_ingredients[id].name);
                snprintf(text, sizeof(text), "%s",
                         u->excluded & (UINT64_C(1) << id) ? "不吃" : "可吃");
            } else {
                kv_label(c, s, &a->han15, 20, y + 6, 120, 28, INK, names[index]);
                switch (index) {
                    case 0:
                        snprintf(text, sizeof(text), "%s", u->sound ? "开" : "关");
                        break;
                    case 1:
                        snprintf(text, sizeof(text), "%u%%", u->volume);
                        break;
                    case 2: {
                        unsigned n = 0;
                        uint64_t bits = u->excluded;
                        while (bits) {
                            n += (unsigned)(bits & 1u);
                            bits >>= 1;
                        }
                        snprintf(text, sizeof(text), "%u 样", n);
                        break;
                    }
                    case 3:
                        snprintf(text, sizeof(text), "%s", u->less_spicy ? "开" : "关");
                        break;
                    case 4:
                        snprintf(text, sizeof(text), "%s", u->less_oil ? "开" : "关");
                        break;
                    case 5:
                        snprintf(text, sizeof(text), "%u", u->difficulty);
                        break;
                    default:
                        snprintf(text, sizeof(text), u->clear_stage ? "确认 %u/2" : "需两次确认",
                                 u->clear_stage);
                        break;
                }
            }
            kv_label(c, s, &a->han12, 164, y + 8, 62, 22, INK, text);
        }
        if (u->settings_mode)
            kv_label(c, s, &a->han12, 10, 278, 220, 14, PAPER,
                     "仅筛食材，调料及交叉接触不筛查");
        kv_footer(c, s, u->settings_mode ? "↑↓ 选择 · OK 切换 · 双 ↑ 返回" : "↑↓ 选择 · OK 调整");
    }
}
