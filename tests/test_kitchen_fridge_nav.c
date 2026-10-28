/* Five-category navigation and refrigerator recommendations, host only. */
#include "kitchen_ui.h"
#include "kitchen_metadata.h"
#include "kitchen_save.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static kc_item items[32];
static kc_catalog catalog = {items, 32, 32};
static krc_pack pack;
static unsigned beef, pork, fish, shrimp, squid, tofu, egg, tomato, rice, noodle;
static uint8_t order[64], fridge_order[10];

static unsigned id(const char* name) {
    for (unsigned i = 0; i < km_ingredient_count; ++i)
        if (!strcmp(km_ingredients[i].id, name)) return i;
    assert(!"missing test ingredient");
    return 0;
}
static uint64_t bit(unsigned n) { assert(n < 64); return UINT64_C(1) << n; }
int kc_read_step(const krc_pack* p, unsigned r, unsigned step, kc_step* out, unsigned* count) {
    (void)p; (void)r; (void)step; (void)out; (void)count; return 0;
}
int kc_read_ingredients(const krc_pack* p, unsigned recipe, kc_ingredient* out,
                        unsigned capacity, unsigned* count) {
    (void)p;
    if (recipe != 28 || capacity < 3) return 0;
    out[0] = (kc_ingredient){.id = beef, .main = 1};
    out[1] = (kc_ingredient){.id = beef, .main = 1};
    out[2] = (kc_ingredient){.id = shrimp, .main = 1};
    *count = 3;
    return 1;
}
static void setup(void) {
    assert(km_ingredient_count <= 64);
    beef=id("beef"); pork=id("pork_lean"); fish=id("fish_whole"); shrimp=id("shrimp");
    squid=id("squid"); tofu=id("tofu"); egg=id("egg"); tomato=id("tomato");
    rice=id("rice"); noodle=id("noodle");
    for (unsigned i = 0; i < km_ingredient_count; ++i) order[i] = (uint8_t)i;
    uint8_t f[] = {beef, pork, fish, shrimp, squid, tofu, egg, tomato, rice, noodle};
    memcpy(fridge_order, f, sizeof(f));
    for (unsigned i = 0; i < 32; ++i) {
        items[i].hash = i + 1;
        items[i].recipe = (kr_recipe){.ingredients=bit(beef), .main_ingredients=bit(beef),
                                      .minutes=10, .weight=100, .courses=KR_MEAT};
        items[i].difficulty=1; items[i].servings=1;
    }
}
static ku_ui fresh(unsigned n) {
    ku_ui u;
    assert(ku_init(&u, &catalog, &pack, 20000, 1, 1, order, km_ingredient_count,
                   fridge_order, n));
    return u;
}
static void key(ku_ui* u, ki_key key_value, ki_kind kind) { ku_key(u, key_value, kind, 1000); }
static void navigation(void) {
    ku_ui u = fresh(10);
    u.focus=1; key(&u, KI_OK, KI_SHORT);
    assert(u.page==9 && !u.fridge_selecting_items && u.fridge_category==0 && u.focus==0);
    key(&u, KI_DOWN, KI_SHORT); assert(u.fridge_category==1 && u.focus==1 && !u.dirty);
    key(&u, KI_OK, KI_SHORT); assert(u.fridge_selecting_items && u.focus==2);
    key(&u, KI_DOWN, KI_SHORT); assert(u.focus==3);
    key(&u, KI_OK, KI_SHORT); assert(u.fridge==bit(shrimp) && u.dirty);
    key(&u, KI_DOWN, KI_SHORT); assert(u.focus==4);
    key(&u, KI_DOWN, KI_SHORT); assert(u.focus==2); /* wraps only within seafood */
    key(&u, KI_UP, KI_DOUBLE); assert(!u.fridge_selecting_items && u.focus==1);
    for (unsigned i=0; i<3; ++i) key(&u, KI_DOWN, KI_SHORT);
    assert(u.focus==4 && u.fridge_category==4);
    key(&u, KI_OK, KI_SHORT); assert(u.fridge_selecting_items && u.focus==8);
    key(&u, KI_DOWN, KI_SHORT); assert(u.focus==9);
    key(&u, KI_OK, KI_SHORT); assert(u.fridge==(bit(shrimp)|bit(noodle)));
    key(&u, KI_OK, KI_LONG); assert(u.page==10);
    key(&u, KI_UP, KI_DOUBLE);
    assert(u.page==9 && u.fridge_category==4 && u.fridge_selecting_items && u.focus==9);
    key(&u, KI_UP, KI_DOUBLE); assert(!u.fridge_selecting_items && u.focus==4);
    key(&u, KI_DOWN, KI_SHORT); assert(u.focus==0); /* 5 tabs wrap */
    key(&u, KI_UP, KI_DOUBLE); assert(u.page==1);

    ku_ui empty_staples = fresh(8);
    empty_staples.focus=1; key(&empty_staples, KI_OK, KI_SHORT);
    key(&empty_staples, KI_UP, KI_SHORT); /* meat -> staples */
    assert(empty_staples.fridge_category==4);
    key(&empty_staples, KI_OK, KI_SHORT);
    assert(!empty_staples.fridge_selecting_items && empty_staples.focus==4);
    assert(!empty_staples.fridge && !empty_staples.dirty);

    /* Navigational focus is transient, never part of the KST1 record. */
    ku_ui a=fresh(10), b=a;
    b.fridge_category=4; b.fridge_selecting_items=1; b.focus=9; b.page=9;
    uint8_t x[KU_SAVE_MAX],y[KU_SAVE_MAX];
    size_t nx=ku_save_encode(&a,x,sizeof(x),1,0,-1), ny=ku_save_encode(&b,y,sizeof(y),1,0,-1);
    assert(nx && nx==ny && !memcmp(x,y,nx));
}
static void matching(void) {
    ku_ui u=fresh(10); uint16_t found[KR_MAX_RECIPES];
    assert(ku_fridge_matches(&u,found,KR_MAX_RECIPES)==0);
    u.fridge=bit(beef);
    for (unsigned i=20; i<25; ++i)
        items[i].recipe.ingredients=items[i].recipe.main_ingredients=bit(beef)|bit(shrimp);
    items[25].recipe.ingredients=items[25].recipe.main_ingredients=bit(shrimp);
    items[26].recipe.ingredients=items[26].recipe.main_ingredients=bit(beef)|bit(shrimp)|bit(fish);
    items[27].recipe.ingredients=bit(beef)|bit(tomato);
    u.excluded=bit(tomato);
    items[28].recipe.ingredients=items[28].recipe.main_ingredients=bit(beef)|bit(shrimp);
    items[28].recipe.repeated_main=1;
    for (unsigned i=29; i<32; ++i) items[i].recipe.ingredients=items[i].recipe.main_ingredients=bit(shrimp);
    unsigned n=ku_fridge_matches(&u,found,KR_MAX_RECIPES);
    assert(n==26 && ku_fridge_matches(&u,NULL,0)==26);
    uint16_t short_list[10]; assert(ku_fridge_matches(&u,short_list,10)==26);
    for (unsigned i=0; i<20; ++i) assert(found[i]==i && short_list[i%10]==i%10);
    for (unsigned i=20; i<n; ++i) assert(found[i]!=25 && found[i]!=26 && found[i]!=27);
    assert(found[20]==28); /* 2/3 beats 1/2, after all complete dishes */
}
int main(void) { setup(); navigation(); matching(); puts("PASS: five fridge categories, preserved selections and full matching"); }
