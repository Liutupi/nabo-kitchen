#include "kitchen_ui.h"
#include <limits.h>
#include <string.h>
#include "kitchen_metadata.h"
#include "kitchen_model.h"

void ku_acknowledge_clock_loss(ku_ui* u) {
    if (!u)
        return;
    u->message = KU_NO_MESSAGE;
    u->message_due = 0;
    u->page = 7;
    u->focus = 0;
    u->stack_count = 0;
    for (unsigned i = 0; i < u->timers.count; ++i)
        if (u->timers.items[i].status == KT_PAUSED) {
            u->focus = (uint8_t)i;
            break;
        }
    /* Persist the recovered PAUSED state even if the warning was the only
     * change. Otherwise a reset can recover the old RUNNING record again. */
    u->dirty = 1;
}

unsigned ku_fridge_category_items(const ku_ui* u, unsigned category, uint8_t* positions,
                                  unsigned capacity) {
    static const char* const names[KU_FRIDGE_CATEGORIES] = {"meat", "seafood", "egg_tofu", "veg",
                                                            "staple"};
    if (!u || category >= KU_FRIDGE_CATEGORIES || (!positions && capacity))
        return 0;
    unsigned count = 0;
    for (unsigned i = 0; i < u->fridge_count; ++i) {
        unsigned id = u->fridge_order[i];
        if (id < km_ingredient_count && km_ingredients[id].category &&
            !strcmp(km_ingredients[id].category, names[category])) {
            if (count < capacity)
                positions[count] = (uint8_t)i;
            ++count;
        }
    }
    return count;
}

static void go(ku_ui* u, unsigned page, int push, unsigned focus) {
    if (push && u->stack_count >= 16) {
        u->message = KU_CAPACITY_ERROR;
        return;
    }
    if (push)
        u->stack[u->stack_count++] = (ku_location){u->page, u->focus, u->recipe};
    u->page = (uint8_t)page;
    u->focus = (uint8_t)focus;
    u->clear_stage = 0;
    u->settings_mode = 0;
    if (page == 9) {
        u->fridge_category = 0;
        u->fridge_selecting_items = 0;
    }
    u->step_scroll = u->step_scroll_max = u->paper_scroll = u->paper_scroll_max = 0;
}
static void home(ku_ui* u) {
    u->stack_count = 0;
    u->outside = 0;
    go(u, 1, 0, 0);
}
static void back(ku_ui* u) {
    if (u->page == 13 && u->settings_mode) {
        u->settings_mode = 0;
        u->focus = 2;
        return;
    }
    if (u->stack_count) {
        ku_location x = u->stack[--u->stack_count];
        u->page = x.page;
        u->focus = x.focus;
        u->recipe = x.recipe;
        u->step_scroll = u->step_scroll_max = u->paper_scroll = u->paper_scroll_max = 0;
        u->clear_stage = 0;
    } else
        home(u);
}
static unsigned recent_age(const ku_ui* u, unsigned recipe) {
    return ks_recent_age(&u->store, recipe);
}
static void show_result(ku_ui* u, int push) {
    u->result_focus = 0;
    u->recipe = u->result[0];
    go(u, 4, push, 0);
}
static void draw(ku_ui* u, int reroll, int64_t now) {
    if (!reroll && u->has_saved) {
        u->result_count = u->saved_count;
        u->notes = u->saved_notes;
        memcpy(u->result, u->saved_result, sizeof(u->result));
        show_result(u, u->page != 3);
        return;
    }
    int replacing = reroll || u->has_saved;
    kr_recipe recipes[KR_MAX_RECIPES];
    for (unsigned i = 0; i < u->catalog->count; ++i) {
        const kc_item* item = &u->catalog->items[i];
        recipes[i] = item->recipe;
        recipes[i].weight =
            (uint16_t)kitchen_recipe_weight(recent_age(u, i), ku_recipe_bit(u->favorites, i),
                                            0,
                                            ku_recipe_bit(u->wanted, i) || !!(item->tags & (1u << 12)),
                                            item->difficulty > u->difficulty);
    }
    uint32_t seed = u->daily_seed + u->draw_count * UINT32_C(0x9e3779b9);
    const uint16_t* avoid = u->result_count ? u->result : u->saved_result;
    unsigned avoid_count = replacing ? (u->result_count ? u->result_count : u->saved_count) : 0;
    kr_result r = kitchen_recommend(recipes, u->catalog->count, u->excluded, u->conditions.combo,
                                    u->conditions.minutes, seed, avoid, avoid_count);
    if (r.status != KR_OK) {
        u->message = r.status == KR_NO_ALLOWED       ? KU_NO_RECIPES
                     : r.status == KR_NO_COMBINATION ? KU_NO_COMBO
                                                     : KU_DATA_ERROR;
        return;
    }
    ++u->draw_count;
    if (replacing && u->rerolls_used < UINT8_MAX) ++u->rerolls_used;
    u->saved_count = r.count;
    u->saved_notes = r.notes;
    memcpy(u->saved_result, r.indices, sizeof(u->saved_result));
    u->saved_conditions = u->conditions;
    u->has_saved = 1;
    u->dirty = 1;
    go(u, 3, u->page != 4, 0);
    u->transition_due = now + 1600;
}
int ku_init(ku_ui* u, const kc_catalog* catalog, const krc_pack* pack, int32_t day, unsigned month,
            uint32_t seed, const uint8_t* order, unsigned ingredients, const uint8_t* fridge_order,
            unsigned fridge_count) {
    if (!u || !catalog || !pack || !catalog->items || !catalog->count ||
        catalog->count > KR_MAX_RECIPES || month < 1 || month > 12 || ingredients > 64 ||
        fridge_count > ingredients || (!order && ingredients) || (!fridge_order && fridge_count))
        return 0;
    uint64_t seen = 0;
    for (unsigned i = 0; i < ingredients; ++i) {
        if (order[i] >= ingredients || (seen & (UINT64_C(1) << order[i])))
            return 0;
        seen |= UINT64_C(1) << order[i];
    }
    seen = 0;
    for (unsigned i = 0; i < fridge_count; ++i) {
        if (fridge_order[i] >= ingredients || (seen & (UINT64_C(1) << fridge_order[i])))
            return 0;
        seen |= UINT64_C(1) << fridge_order[i];
    }
    memset(u, 0, sizeof(*u));
    u->catalog = catalog;
    u->pack = pack;
    u->day = day;
    u->month = (uint8_t)month;
    u->daily_seed = seed;
    u->conditions = (ku_conditions){2, 30, 0};
    u->difficulty = 2;
    u->reroll_limit = 3;
    u->volume = 50;
    u->page = 1;
    u->ingredient_count = (uint8_t)ingredients;
    u->fridge_count = (uint8_t)fridge_count;
    if (ingredients)
        memcpy(u->ingredient_order, order, ingredients);
    if (fridge_count)
        memcpy(u->fridge_order, fridge_order, fridge_count);
    ks_init(&u->store);
    kt_init(&u->timers);
    return 1;
}
void ku_set_day(ku_ui* u, int32_t day, unsigned month, uint32_t seed) {
    /* Kept for old host callers; dates never alter a v2 menu or seed. */
    (void)u; (void)day; (void)month; (void)seed;
}
void ku_scroll_extents(ku_ui* u, unsigned step_max, unsigned paper_max) {
    if (!u)
        return;
    u->step_scroll_max = (uint16_t)(step_max > UINT16_MAX ? UINT16_MAX : step_max);
    u->paper_scroll_max = (uint16_t)(paper_max > UINT16_MAX ? UINT16_MAX : paper_max);
    if (u->step_scroll > u->step_scroll_max)
        u->step_scroll = u->step_scroll_max;
    if (u->paper_scroll > u->paper_scroll_max)
        u->paper_scroll = u->paper_scroll_max;
}
void ku_tick(ku_ui* u, int64_t now) {
    if (!u)
        return;
    if (kt_tick(&u->timers, now))
        u->dirty = 1;
    if (u->message) {
        if (!u->message_due)
            u->message_due = now + 2400;
        else if (now >= u->message_due) {
            u->message = KU_NO_MESSAGE;
            u->message_due = 0;
        }
    }
    /* The boot timeline ends at transition_due, but its final scene remains
     * visible until the user explicitly enters the kitchen. */
    if (u->page == 3 && now >= u->transition_due && u->saved_count) {
        u->result_count = u->saved_count;
        u->notes = u->saved_notes;
        memcpy(u->result, u->saved_result, sizeof(u->result));
        show_result(u, 0);
    }
}
void ku_startup(ku_ui* u, int64_t now) {
    if (u) {
        u->page = 0;
        u->transition_due = now + KU_BOOT_DURATION_MS;
    }
}
static void step_next(ku_ui* u, int64_t now) {
    if (u->recipe >= u->catalog->count) {
        u->message = KU_DATA_ERROR;
        return;
    }
    ks_session* s = &u->sessions[u->recipe];
    kc_step step;
    unsigned count;
    if (!kc_read_step(u->pack, u->recipe, s->index, &step, &count)) {
        u->message = KU_DATA_ERROR;
        return;
    }
    uint16_t bit = (uint16_t)(1u << s->index);
    if (step.timer_seconds && !(u->timer_steps[u->recipe] & bit)) {
        for (unsigned i = 0; i < 3; ++i)
            if (u->timer_ids[i] && u->timer_recipe[i] == u->recipe &&
                u->timer_step[i] == s->index) {
                u->timer_steps[u->recipe] |= bit;
                u->dirty = 1;
                return;
            }
        unsigned slot = 0;
        while (slot < 3 && u->timer_ids[slot])
            ++slot;
        if (slot == 3) {
            u->message = KU_NO_TIMER_SLOT;
            return;
        }
        uint32_t id;
        if (!kt_add(&u->timers, step.timer_seconds, now, &id)) {
            u->message = KU_NO_TIMER_SLOT;
            return;
        }
        u->timer_ids[slot] = id;
        u->timer_recipe[slot] = u->recipe;
        u->timer_step[slot] = s->index;
        u->timer_steps[u->recipe] |= bit;
        u->dirty = 1;
        return;
    }
    ks_result result = ks_complete_step(&u->store, s, u->recipe, count,
                                        u->date_unconfirmed ? INT32_MIN : u->day);
    u->dirty = 1;
    if (result == KS_COMPLETE || result == KS_ALREADY_COMPLETE) {
        u->has_saved = 0;
        u->saved_count = 0;
        go(u, 8, 1, 0);
    } else if (result == KS_INCOMPLETE)
        u->message = KU_STEP_INCOMPLETE;
    else if (result == KS_FULL)
        u->message = KU_CAPACITY_ERROR;
    else if (result == KS_INVALID)
        u->message = KU_DATA_ERROR;
}
void ku_key(ku_ui* u, ki_key key, ki_kind kind, int64_t now) {
    if (!u || key < KI_UP || key >= KI_KEYS || kind < KI_SHORT || kind > KI_LONG)
        return;
    u->message = KU_NO_MESSAGE;
    u->message_due = 0;
    ku_tick(u, now);
    if (key == KI_OK && kind == KI_SHORT && u->page != 7) {
        uint32_t alarm = kt_alarm(&u->timers, now);
        if (alarm) {
            unsigned i = 0;
            while (i < u->timers.count && u->timers.items[i].id != alarm)
                ++i;
            u->outside = 0;
            go(u, 7, 1, i);
            return;
        }
    }
    if (u->page == 0) {
        if (key == KI_OK && kind == KI_SHORT)
            home(u);
        return;
    }
    if (u->outside) {
        if (key == KI_UP && kind == KI_LONG)
            return;
        home(u);
        return;
    }
    if (key == KI_UP && kind == KI_LONG) {
        u->outside = 1;
        u->dirty = 1;
        return;
    }
    if (key == KI_UP && kind == KI_DOUBLE) {
        if (u->page == 9 && u->fridge_selecting_items) {
            u->fridge_selecting_items = 0;
            u->focus = u->fridge_category;
            return;
        }
        back(u);
        return;
    }
    if (u->page == 9) {
        if (key == KI_OK && kind == KI_LONG) {
            go(u, 10, 1, 0);
            return;
        }
        if (kind != KI_SHORT)
            return;
        if (!u->fridge_selecting_items) {
            if (key == KI_UP || key == KI_DOWN) {
                unsigned delta = key == KI_UP ? KU_FRIDGE_CATEGORIES - 1 : 1;
                u->focus = (uint8_t)((u->focus + delta) % KU_FRIDGE_CATEGORIES);
                u->fridge_category = u->focus;
            } else if (key == KI_OK) {
                uint8_t positions[64];
                unsigned count = ku_fridge_category_items(u, u->fridge_category, positions, 64);
                if (count) {
                    u->fridge_selecting_items = 1;
                    u->focus = positions[0];
                }
            }
        } else {
            uint8_t positions[64];
            unsigned count = ku_fridge_category_items(u, u->fridge_category, positions, 64);
            if (!count) {
                u->fridge_selecting_items = 0;
                u->focus = u->fridge_category;
            } else if (key == KI_UP || key == KI_DOWN) {
                unsigned pos = 0;
                while (pos < count && positions[pos] != u->focus)
                    ++pos;
                if (pos == count)
                    pos = 0;
                else
                    pos = (pos + (key == KI_UP ? count - 1 : 1)) % count;
                u->focus = positions[pos];
            } else if (key == KI_OK && u->focus < u->fridge_count) {
                u->fridge ^= UINT64_C(1) << u->fridge_order[u->focus];
                u->dirty = 1;
            }
        }
        return;
    }
    if (kind == KI_LONG) {
        if (key == KI_OK && u->page == 1)
            draw(u, 0, now);
        else if (key == KI_DOWN && u->page == 6)
            go(u, 5, 1, 0);
        else if (key == KI_DOWN && u->page == 5) {
            u->paper_scroll =
                u->paper_scroll + 120 >= u->paper_scroll_max ? 0 : u->paper_scroll + 120;
        } else if (key == KI_OK && u->page == 7 && u->focus < u->timers.count) {
            kt_toggle(&u->timers, u->timers.items[u->focus].id, now);
            u->dirty = 1;
        } else if (key == KI_DOWN && u->page == 7 && u->focus < u->timers.count) {
            /* Cancel only the selected timer. Its step remains marked as
             * started so returning to the recipe cannot restart it silently. */
            uint32_t id = u->timers.items[u->focus].id;
            kt_cancel(&u->timers, id);
            for (unsigned i = 0; i < 3; ++i)
                if (u->timer_ids[i] == id)
                    u->timer_ids[i] = 0;
            if (u->focus >= u->timers.count && u->focus)
                --u->focus;
            u->dirty = 1;
        } else if (key == KI_OK && u->page == 4) {
            ku_recipe_flip(u->favorites, u->recipe);
            u->dirty = 1;
        } else if (key == KI_OK && u->page == 12 && u->focus) {
            uint16_t list[KR_MAX_RECIPES];
            unsigned count = ku_book_list(u, list, KR_MAX_RECIPES);
            if (count != UINT_MAX && (unsigned)u->focus - 1 < count) {
                ku_recipe_flip(u->favorites, list[u->focus - 1]);
                u->dirty = 1;
            }
        }
        return;
    }
    if (u->page == 6) {
        if (key == KI_OK)
            step_next(u, now);
        else if (key == KI_UP && u->sessions[u->recipe].index) {
            --u->sessions[u->recipe].index;
            u->dirty = 1;
        } else if (key == KI_DOWN) {
            if (u->step_scroll < u->step_scroll_max) {
                unsigned next = u->step_scroll + 50;
                u->step_scroll = (uint16_t)(next > u->step_scroll_max ? u->step_scroll_max : next);
            } else
                go(u, 7, 1, 0);
        }
        return;
    }
    if (u->page == 4) {
        if (key == KI_DOWN)
            draw(u, 1, now);
        else if (key == KI_UP) {
            if (u->result_count > 1) {
                u->result_focus = (u->result_focus + u->result_count - 1) % u->result_count;
                u->recipe = u->result[u->result_focus];
            } else
                home(u);
        } else if (key == KI_OK)
            go(u, 5, 1, 0);
        return;
    }
    if (u->page == 5) {
        if (key == KI_OK) {
            if (u->sessions[u->recipe].completed) {
                ks_reset_session(&u->sessions[u->recipe]);
                u->timer_steps[u->recipe] = 0;
            }
            go(u, 6, 1, 0);
            u->dirty = 1;
        } else if (key == KI_DOWN && !ku_add_shopping(u, u->recipe))
            u->message = KU_CAPACITY_ERROR;
        else if (key == KI_UP)
            u->paper_scroll = u->paper_scroll > 120 ? u->paper_scroll - 120 : 0;
        return;
    }
    if (u->page == 8) {
        if (key == KI_OK)
            home(u);
        return;
    }
    if (u->page == 7) {
        if (key == KI_OK && u->focus < u->timers.count &&
            u->timers.items[u->focus].status == KT_DONE) {
            uint32_t id = u->timers.items[u->focus].id;
            kt_acknowledge(&u->timers, id);
            kt_cancel(&u->timers, id);
            for (unsigned i = 0; i < 3; ++i)
                if (u->timer_ids[i] == id)
                    u->timer_ids[i] = 0;
            u->focus = 0;
            u->dirty = 1;
        } else if ((key == KI_UP || key == KI_DOWN) && u->timers.count)
            u->focus = (u->focus + (key == KI_UP ? u->timers.count - 1 : 1)) % u->timers.count;
        return;
    }
    unsigned count = u->page == 1    ? 5
                     : u->page == 2  ? 4
                     : u->page == 11 ? u->shopping_count + 1
                     : u->page == 13 ? (u->settings_mode ? u->ingredient_count : 7)
                                     : 1;
    uint16_t list[KR_MAX_RECIPES];
    if (u->page == 10) {
        count = ku_fridge_matches(u, list, KR_MAX_RECIPES);
        if (count == UINT_MAX) {
            u->message = KU_DATA_ERROR;
            return;
        }
    }
    if (u->page == 12) {
        count = ku_book_list(u, list, KR_MAX_RECIPES);
        if (count == UINT_MAX) {
            u->message = KU_DATA_ERROR;
            return;
        }
        ++count;
    }
    if (!count)
        count = 1;
    if (key == KI_UP || key == KI_DOWN) {
        u->focus = (u->focus + (key == KI_UP ? count - 1 : 1)) % count;
        u->clear_stage = 0;
        return;
    }
    if (key != KI_OK)
        return;
    if (u->page == 1) {
        static const uint8_t pages[] = {2, 9, 11, 12, 13};
        go(u, pages[u->focus], 1, 0);
    } else if (u->page == 2) {
        if (u->focus == 0)
            u->conditions.people = u->conditions.people % 5 + 1;
        else if (u->focus == 1) {
            static const uint8_t values[] = {15, 30, 60, 0};
            unsigned i = 0;
            while (i < 4 && values[i] != u->conditions.minutes)
                ++i;
            u->conditions.minutes = values[(i + 1) % 4];
        } else if (u->focus == 2)
            u->conditions.combo = (u->conditions.combo + 1) % 4;
        else {
            draw(u, 0, now);
            return;
        }
        u->dirty = 1;
    } else if (u->page == 10) {
        unsigned n = ku_fridge_matches(u, list, KR_MAX_RECIPES);
        if (n != UINT_MAX && u->focus < n) {
            u->result[0] = list[u->focus];
            u->result_count = 1;
            u->notes = 0;
            show_result(u, 1);
        }
    } else if (u->page == 11) {
        uint8_t order[KU_SHOPPING_CAPACITY];
        ku_shopping_order(u, order, KU_SHOPPING_CAPACITY);
        if (u->focus < u->shopping_count)
            u->shopping[order[u->focus]].bought ^= 1;
        else {
            unsigned w = 0;
            for (unsigned i = 0; i < u->shopping_count; ++i)
                if (!u->shopping[i].bought)
                    u->shopping[w++] = u->shopping[i];
            u->shopping_count = (uint8_t)w;
            if (u->focus > w)
                u->focus = (uint8_t)w;
        }
        u->dirty = 1;
    } else if (u->page == 12) {
        unsigned n = ku_book_list(u, list, KR_MAX_RECIPES);
        if (!u->focus) {
            u->book_filter = (u->book_filter + 1) % 4;
            u->focus = 0;
        } else if (n != UINT_MAX && (unsigned)u->focus - 1 < n) {
            u->result[0] = list[u->focus - 1];
            u->result_count = 1;
            u->notes = 0;
            show_result(u, 1);
        }
    } else if (u->page == 13) {
        if (u->settings_mode) {
            if (u->focus < u->ingredient_count) {
                u->excluded ^= UINT64_C(1) << u->ingredient_order[u->focus];
                u->dirty = 1;
            }
            return;
        }
        if (u->focus != 6)
            u->clear_stage = 0;
        switch (u->focus) {
            case 0:
                u->sound ^= 1;
                break;
            case 1:
                u->volume = (u->volume + 10) % 110;
                break;
            case 2:
                u->settings_mode = 1;
                u->focus = 0;
                break;
            case 3:
                u->less_spicy ^= 1;
                break;
            case 4:
                u->less_oil ^= 1;
                break;
            case 5:
                u->difficulty = u->difficulty % 3 + 1;
                break;
            case 6:
                if (++u->clear_stage == 3) {
                    ks_init(&u->store);
                    memset(u->sessions, 0, sizeof(u->sessions));
                    memset(u->timer_steps, 0, sizeof(u->timer_steps));
                    u->clear_stage = 0;
                }
                break;
            default:
                break;
        }
        u->dirty = 1;
    }
}
