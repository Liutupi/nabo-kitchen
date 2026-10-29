#include "kitchen_view.h"
#include "kitchen_metadata.h"
#include "kitchen_calendar_view.h"
#include "kitchen_view_internal.h"
#include <stdio.h>
#include <string.h>
static krc_pack recipes;static kc_item items[KR_MAX_RECIPES];static kc_catalog catalog={items,KR_MAX_RECIPES,0};
static kfn_pack fonts;static ksp_pack dishes,icons,ui;static kan_pack animation;static kan_action idle;
static kv_assets assets;static ku_ui state;static kv_scene scene;static uint8_t frame[192*192];
static void catalog_progress(void *context){++*(unsigned*)context;}
int view_catalog_cooperative(void){
    kc_item scratch_items[KR_MAX_RECIPES]={0};
    kc_catalog scratch={scratch_items,KR_MAX_RECIPES,0};unsigned calls=0;
    if(!kc_load_cooperative(&scratch,&recipes,catalog_progress,&calls))return 0;
    return scratch.count==catalog.count&&calls==catalog.count&&
        !memcmp(scratch.items,catalog.items,catalog.count*sizeof(kc_item));
}
int view_open(const uint8_t *r,size_t rn,const uint8_t *f,size_t fn,const uint8_t *d,size_t dn,
              const uint8_t *i,size_t in,const uint8_t *u,size_t un,const uint8_t *a,size_t an){
    if(!krc_open(&recipes,r,rn)||!kc_load(&catalog,&recipes)||!kfn_open(&fonts,f,fn)||
       !ksp_open(&dishes,d,dn)||!ksp_open(&icons,i,in)||!ksp_open(&ui,u,un)||!kan_open(&animation,a,an)||
       !kan_find(&animation,"idle",&idle)||animation.width!=192||animation.height!=192)return 0;
    kan_player player;if(!kan_start(&player,&animation,&idle,frame,sizeof(frame))||kan_next(&player)!=1)return 0;
    return kv_assets_init(&assets,&catalog,&recipes,&fonts,&dishes,&icons,&ui,&animation);
}
int view_page(unsigned page,unsigned recipe){
    if(page>13||recipe>=catalog.count)return 0;
    if(!ku_init(&state,&catalog,&recipes,20754,10,42,km_ingredient_order,km_ingredient_order_count,km_fridge_order,km_fridge_order_count))return 0;
    state.page=(uint8_t)page;state.recipe=(uint16_t)recipe;
    if(page==4){state.result[0]=(uint16_t)recipe;state.result_count=1;}
    if(page==7){uint32_t id;if(!kt_add(&state.timers,90,1000,&id))return 0;}
    if(page==10)state.fridge=UINT64_MAX;
    if(page==11&&!ku_add_shopping(&state,recipe))return 0;
    if(page==12){state.store.cooked[0]=(ks_cooked){(uint16_t)recipe,1,20754};state.store.cooked_count=1;}
    return kv_prepare(&scene,&state,&assets,1000,80,"20:30",frame);
}
int view_draw(uint16_t *pixels,int y,int height){kg_canvas c;kg_init(&c,pixels,240,y,height,0x18c6);
    if (!kv_draw(&c,&scene)) return 0;
    if (state.page == 1) kcal_view_hint(&c,&scene,state.date_unconfirmed);
    return !c.error;
}
int view_step(unsigned step){state.page=6;state.sessions[state.recipe].index=(uint8_t)step;
    return kv_prepare(&scene,&state,&assets,1000,80,"20:30",frame);}
int view_boot_at(unsigned elapsed){
    ku_startup(&state,1000);
    return kv_prepare(&scene,&state,&assets,1000+(int64_t)elapsed,80,"20:30",frame);
}
int view_boot_page(unsigned elapsed){
    ku_startup(&state,1000);
    ku_tick(&state,1000+(int64_t)elapsed);
    return state.page;
}
int view_boot_skip(void){
    ku_startup(&state,1000);
    ku_key(&state,KI_OK,KI_SHORT,1100);
    return state.page;
}
/* Additional visual-state probes; no user data or device interaction. */
int view_combo(unsigned selected) {
    if (selected >= 4 || catalog.count < 4) return 0;
    state.page = 4; state.result_count = 4; state.result_focus = (uint8_t)selected;
    for (unsigned i = 0; i < 4; ++i) state.result[i] = (uint16_t)i;
    state.recipe = state.result[selected];
    return kv_prepare(&scene, &state, &assets, 1000, 80, "20:30", frame);
}
int view_paper_scroll(unsigned offset) {
    if (state.page != 5 || offset > UINT16_MAX) return 0;
    state.paper_scroll = (uint16_t)offset;
    return kv_prepare(&scene, &state, &assets, 1000, 80, "20:30", frame);
}
unsigned view_paper_max(void) { return state.paper_scroll_max; }
int view_step_timer(unsigned kind) {
    if (state.page != 6 || kind > 2) return 0;
    kt_init(&state.timers);memset(state.timer_ids, 0, sizeof(state.timer_ids));
    uint32_t id;if (!kt_add(&state.timers, 90, 1000, &id)) return 0;
    state.timer_ids[0] = id;state.timer_recipe[0] = state.recipe;
    state.timer_step[0] = state.sessions[state.recipe].index;
    if (kind == 1) kt_toggle(&state.timers, id, 21000);
    if (kind == 2 && !kt_tick(&state.timers, 91000)) return 0;
    return kv_prepare(&scene, &state, &assets, kind == 2 ? 91000 : 21000, 80, "20:30", frame);
}
int view_complete_progress(void) {
    if (state.page != 8) return 0;
    state.store.cooked[0] = (ks_cooked){state.recipe, 1, state.day};
    state.store.cooked_count = 1;
    state.store.history[0] = (ks_record){state.recipe, state.day};
    state.store.history_count = 1;
    return kv_prepare(&scene, &state, &assets, 1000, 80, "20:30", frame);
}

int view_scene_time(unsigned elapsed) {
    return kv_prepare(&scene, &state, &assets, 1000 + (int64_t)elapsed, 80, "20:30", frame);
}

int view_finished_alarm(void) {
    kt_init(&state.timers);
    uint32_t id;
    if (!kt_add(&state.timers, 1, 1000, &id) || !kt_tick(&state.timers, 2000)) return 0;
    return kv_prepare(&scene, &state, &assets, 4500, 80, "20:30", frame);
}

unsigned view_count(void) { return catalog.count; }
unsigned view_steps(unsigned recipe) {
    kc_step step;
    unsigned count = 0;
    return kc_read_step(&recipes, recipe, 0, &step, &count) ? count : 0;
}
unsigned view_step_max(void) { return state.step_scroll_max; }
int view_step_scroll(unsigned offset) {
    if (state.page != 6 || offset > state.step_scroll_max) return 0;
    state.step_scroll = (uint16_t)offset;
    return kv_prepare(&scene, &state, &assets, 1000, 80, "20:30", frame);
}
int view_exclusion_settings(void) {
    if (!view_page(13, 0)) return 0;
    state.settings_mode = 1;
    return kv_prepare(&scene, &state, &assets, 1000, 80, "20:30", frame);
}
int view_stripes_match(void) {
    static uint16_t full[240 * 320], part[240 * 8];
    if (!view_draw(full, 0, 320)) return 0;
    for (int y = 0; y < 320; y += 8) {
        if (!view_draw(part, y, 8)) return 0;
        if (memcmp(full + y * 240, part, sizeof(part))) return 0;
    }
    return 1;
}
/* Test hero precedence and the deliberately limited legacy fallback without
 * fabricating a complete PNG or changing the real catalog. */
int view_has_hero(unsigned recipe) {
    if (recipe >= catalog.count) return 0;
    char name[48]; ksp_sprite sprite;
    snprintf(name, sizeof(name), "hero_%s", catalog.items[recipe].id);
    return ksp_find(&dishes, name, &sprite);
}
int view_dish_hero_required(unsigned recipe) {
    return recipe < catalog.count ? assets.dishes[recipe].hero_required : -1;
}
int view_compare_hero_required(unsigned recipe, int expected_equal) {
    static uint16_t before[240 * 320], after[240 * 320];
    if (!view_page(4, recipe)) return 0;
    uint8_t old = assets.dishes[recipe].hero_required;
    assets.dishes[recipe].hero_required = 0;
    int ok = view_draw(before, 0, 320);
    assets.dishes[recipe].hero_required = 1;
    ok = ok && view_draw(after, 0, 320) && view_stripes_match();
    int equal = !memcmp(before, after, sizeof(before));
    assets.dishes[recipe].hero_required = old;
    return ok && equal == !!expected_equal;
}
int view_fridge_category(unsigned category, unsigned item_mode) {
    if (category >= KU_FRIDGE_CATEGORIES || !view_page(9, 0)) return 0;
    state.fridge_category = (uint8_t)category; state.focus = (uint8_t)category;
    if (item_mode) {
        uint8_t positions[64];
        if (ku_fridge_category_items(&state, category, positions, 64)) {
            state.fridge_selecting_items = 1; state.focus = positions[0];
            state.fridge = UINT64_C(1) << state.fridge_order[state.focus];
        }
    }
    return kv_prepare(&scene, &state, &assets, 1000, 80, "20:30", frame) &&
           view_stripes_match();
}
int view_fridge_selection_badge(void) {
    static uint16_t off[240 * 320], on[240 * 320];
    if (!view_fridge_category(0, 1)) return 0;
    state.fridge = 0;
    if (!kv_prepare(&scene, &state, &assets, 1000, 80, "20:30", frame) ||
        !view_draw(off, 0, 320)) return 0;
    state.fridge = UINT64_C(1) << state.fridge_order[state.focus];
    if (!kv_prepare(&scene, &state, &assets, 1000, 80, "20:30", frame) ||
        !view_draw(on, 0, 320) || !view_stripes_match()) return 0;
    /* Gold focus is unchanged; the selected tick must still change the
     * top-right of the focused first ingredient tile (x=8,y=74). */
    for (int y = 77; y < 90; ++y)
        for (int x = 47; x < 60; ++x)
            if (off[y * 240 + x] != on[y * 240 + x]) return 1;
    return 0;
}
int view_fridge_every_ingredient(void) {
    if (!view_page(9, 0)) return 0;
    unsigned tested = 0;
    for (unsigned category = 0; category < KU_FRIDGE_CATEGORIES; ++category) {
        uint8_t positions[64];
        unsigned count = ku_fridge_category_items(&state, category, positions, 64);
        state.fridge_category = (uint8_t)category;
        state.fridge_selecting_items = 1;
        for (unsigned i = 0; i < count; ++i) {
            state.focus = positions[i];
            if (!kv_prepare(&scene, &state, &assets, 1000, 80, "20:30", frame) ||
                !view_stripes_match()) return 0;
            ++tested;
        }
    }
    return (int)tested;
}
int view_fridge_fish_short_name(void) {
    static uint16_t screen[240 * 320], expected[52 * 16];
    if (!view_page(9, 0)) return 0;
    unsigned fish = km_ingredient_count;
    for (unsigned i = 0; i < km_ingredient_count; ++i)
        if (!strcmp(km_ingredients[i].id, "fish_whole")) { fish = i; break; }
    if (fish == km_ingredient_count || strcmp(km_ingredients[fish].name, "鱼（鲈鱼等）"))
        return 0; /* full metadata and stable ID stay intact */
    uint8_t positions[64];
    unsigned count = ku_fridge_category_items(&state, 1, positions, 64), pos = 0;
    while (pos < count && state.fridge_order[positions[pos]] != fish) ++pos;
    if (pos == count) return 0;
    state.fridge_category = 1; state.fridge_selecting_items = 1;
    state.focus = positions[pos];
    if (!kv_prepare(&scene, &state, &assets, 1000, 80, "20:30", frame) ||
        !view_draw(screen, 0, 320) || !view_stripes_match()) return 0;
    kg_canvas c; kg_init(&c, expected, 52, 0, 16, GOLD);
    kg_clip(&c, 2, 0, 48, 16);
    if (kg_text_line(&c, &fonts, &assets.han12, 2, 0, 48,
                     assets.han12.line_height, INK, "鱼") < 0) return 0;
    int x = 8 + (int)(pos % 8 % 4) * 58, y = 74 + (int)(pos % 8 / 4) * 72 + 44;
    for (int row = 0; row < 16; ++row)
        for (int col = 2; col < 46; ++col)
            if (screen[(y + row) * 240 + x + col] != expected[row * 52 + col]) return 0;
    return 1;
}
/* Exercise actual catalog entries after the original 64-bit boundary. */
int view_book_boundary(unsigned recipe) {
    if (recipe < 64 || recipe >= catalog.count || !view_page(12, recipe)) return 0;
    state.book_filter = 2;
    ku_recipe_flip(state.favorites, recipe);
    uint16_t list[KR_MAX_RECIPES];
    if (ku_book_list(&state, list, KR_MAX_RECIPES) != 1 || list[0] != recipe) return 0;
    state.focus = 1;
    if (!kv_prepare(&scene, &state, &assets, 1000, 80, "20:30", frame) ||
        !view_stripes_match()) return 0;
    ku_key(&state, KI_OK, KI_SHORT, 1001);
    if (state.page != 4 || state.result_count != 1 || state.result[0] != recipe) return 0;
    return kv_prepare(&scene, &state, &assets, 1000, 80, "20:30", frame) &&
           view_stripes_match();
}
int view_actual_draw(unsigned seed, unsigned combo) {
    if (combo > 3 || !view_page(2, 0)) return 0;
    state.daily_seed = seed;
    state.conditions.combo = (uint8_t)combo;
    state.conditions.minutes = 0;
    state.focus = 3;
    ku_key(&state, KI_OK, KI_SHORT, 1000);
    if (state.page != 3 || state.saved_count != (combo ? combo + 1 : 1)) return 0;
    ku_tick(&state, 2600);
    if (state.page != 4 || state.result_count != state.saved_count) return 0;
    for (unsigned i = 0; i < state.result_count; ++i) {
        if (state.result[i] >= catalog.count) return 0;
        for (unsigned j = 0; j < i; ++j)
            if (state.result[i] == state.result[j]) return 0;
        unsigned course = catalog.items[state.result[i]].recipe.courses;
        if (combo && ((i == 0 || (combo == 3 && i == 1)) ? !(course & KR_MEAT)
                       : (i == state.result_count - 1 && combo >= 2) ? !(course & KR_SOUP)
                       : !(course & KR_VEG))) return 0;
    }
    return kv_prepare(&scene, &state, &assets, 2600, 80, "20:30", frame) &&
           view_stripes_match();
}
