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
static const char* category(const char* cat) {
    return !strcmp(cat, "meat")       ? "肉"
           : !strcmp(cat, "seafood")  ? "海鲜"
           : !strcmp(cat, "egg_tofu") ? "蛋豆"
                                      : "蔬菜";
}
void kv_lists_draw(kg_canvas* c, const kv_scene* s) {
    const ku_ui* u = s->state;
    const kv_assets* a = s->assets;
    char text[120];
    if (u->page == 9) {
        kv_header(c, s, "冰箱里有啥", BLUE);
        if (!u->fridge_count) {
            kv_footer(c, s, "双击 ↑ 返回");
            return;
        }
        unsigned current = u->fridge_order[u->focus % u->fridge_count];
        const char* cat = km_ingredients[current].category;
        static const char* cats[] = {"meat", "seafood", "egg_tofu", "veg"};
        for (unsigned i = 0; i < 4; ++i) {
            int x = 8 + (int)i * 56;
            kg_round_rect(c, x, 44, 52, 22, 7, !strcmp(cat, cats[i]) ? BLUE : PANEL);
            kv_label(c, s, &a->han12, x + 9, 45, 42, 20, !strcmp(cat, cats[i]) ? INK : PAPER,
                     category(cats[i]));
        }
        unsigned ordered[64], n = 0, pos = 0;
        for (unsigned i = 0; i < u->fridge_count; ++i) {
            unsigned id = u->fridge_order[i];
            if (!strcmp(km_ingredients[id].category, cat)) {
                if (id == current)
                    pos = n;
                ordered[n++] = id;
            }
        }
        unsigned start = pos / 8 * 8;
        for (unsigned j = 0; j < 8 && start + j < n; ++j) {
            unsigned id = ordered[start + j];
            int x = 8 + (int)(j % 4) * 58, y = 74 + (int)(j / 4) * 72;
            kg_round_rect(c, x, y, 52, 66, 8,
                          id == current                       ? GOLD
                          : (u->fridge & (UINT64_C(1) << id)) ? GREEN
                                                              : PAPER2);
            kv_icon(c, s, km_ingredients[id].id, x + 6, y + 3, 40, 40);
            kv_label(c, s, &a->han12, x + 2, y + 44, 48, 20, INK, km_ingredients[id].name);
        }
        kv_icon(c, s, "face_focus", 10, 224, 40, 40);
        kg_round_rect(c, 60, 226, 170, 52, 10, PAPER);
        uint16_t list[10];
        unsigned matches = ku_fridge_matches(u, list, 10);
        if (matches == UINT32_MAX) {
            c->error = 1;
            return;
        }
        unsigned complete = 0;
        for (unsigned i = 0; i < matches; ++i)
            complete += !(u->catalog->items[list[i]].recipe.main_ingredients & ~u->fridge);
        snprintf(text, sizeof(text), "这些能做 %u 道菜", complete);
        kv_label(c, s, &a->han12, 68, 230, 154, 20, INK, text);
        kv_label(c, s, &a->han12, 68, 251, 154, 19, INK, "长按 OK 看推荐");
        kv_footer(c, s, "OK 勾选 · 长 OK 推荐 · 双 ↑ 返回");
        return;
    }
    if (u->page == 10) {
        kv_header(c, s, "冰箱推荐", BLUE);
        unsigned start = (u->focus / 4) * 4;
        if (!s->match_count)
            kv_label(c, s, &a->han16, 15, 96, 210, 65, PAPER, "没有只缺 1 样以内的菜");
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
                kg_round_rect(c, 19, y, 202, 35, 5, PAPER2);
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
        kv_label(c, s, &a->han12, 76, 251, 120, 20, INK, "清空已买");
        kv_footer(c, s, "↑↓ 选择 · OK 打勾 / 清空");
        return;
    }
    if (u->page == 12) {
        kv_header(c, s, "我的菜谱", YELLOW);
        ks_stats stats = ks_get_stats(&u->store, u->day);
        kg_round_rect(c, 8, 44, 224, 52, 9, PAPER2);
        snprintf(text, sizeof(text), "badge_%u", (unsigned)stats.level);
        kv_icon(c, s, text, 14, 50, 40, 40);
        static const char* levels[] = {"", "夜宵学徒", "深夜帮厨", "夜班主厨", "深夜厨神"};
        kv_label(c, s, &a->han16, 62, 46, 160, 26, INK, levels[stats.level]);
        snprintf(text, sizeof(text), "累计 %u 道 · 连续 %u 天", (unsigned)stats.total,
                 (unsigned)stats.streak);
        kv_label(c, s, &a->han12, 62, 73, 160, 20, INK, text);
        static const char* filters[] = {"全部", "做过的", "收藏", "我存的"};
        snprintf(text, sizeof(text), "筛选: %s", filters[u->book_filter]);
        kv_label(c, s, &a->han12, 10, 100, 220, 22, PAPER, text);
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
        static const char* names[] = {"声音", "音量", "每天重抽", "不吃的食材",
                                      "少辣", "少油", "难度",     "清空记录"};
        unsigned total = u->settings_mode ? u->ingredient_count : 8;
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
                    case 2:
                        snprintf(text, sizeof(text), "%u 次", u->reroll_limit);
                        break;
                    case 3: {
                        unsigned n = 0;
                        uint64_t bits = u->excluded;
                        while (bits) {
                            n += (unsigned)(bits & 1u);
                            bits >>= 1;
                        }
                        snprintf(text, sizeof(text), "%u 样", n);
                        break;
                    }
                    case 4:
                        snprintf(text, sizeof(text), "%s", u->less_spicy ? "开" : "关");
                        break;
                    case 5:
                        snprintf(text, sizeof(text), "%s", u->less_oil ? "开" : "关");
                        break;
                    case 6:
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
