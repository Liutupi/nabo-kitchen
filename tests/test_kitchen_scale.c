/* Boundary and adversarial cases for the 192-recipe model. */
#include "kitchen_ui.h"
#include "kitchen_save.h"
#include "kitchen_metadata.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
extern size_t kr_test_visits;

static kc_item items[KR_MAX_RECIPES];
static ku_ui src, dst, scratch;
static uint8_t encoded[KU_SAVE_MAX];
static void setup(void) {
    for (unsigned i = 0; i < KR_MAX_RECIPES; ++i) items[i].hash = i + 100;
    static kc_catalog cat = { items, KR_MAX_RECIPES, KR_MAX_RECIPES };
    src.catalog = dst.catalog = &cat;
    src.conditions = (ku_conditions){2,30,0};
    src.saved_conditions = src.conditions;
    src.month = 10; src.difficulty = 2; src.reroll_limit = 3;
    src.sound = 1; src.volume = 50;
    kt_init(&src.timers);
}
/* Fixture produced by the 128-capacity KST1 encoder with synthetic IDs only. */
static void legacy_128(void) {
    setup();
    kc_catalog old = {items, 128, 128};
    src.catalog = &old;
    for (unsigned i = 0; i < 128; ++i) {
        ku_recipe_flip(src.favorites, i); ku_recipe_flip(src.wanted, i);
        src.sessions[i] = (ks_session){1,0,0}; src.timer_steps[i] = 1;
        src.store.cooked[i] = (ks_cooked){i, i + 1, 20000};
    }
    src.store.cooked_count = 128;
    src.saved_count = 2; src.has_saved = 1; src.saved_result[0] = 63; src.saved_result[1] = 64;
    size_t n = ku_save_encode(&src, encoded, sizeof(encoded), 77, 0, -1);
    FILE* f = fopen("tests/fixtures/kitchen_state_128.kst", "rb"); assert(f);
    uint8_t prior[KU_SAVE_MAX]; size_t old_n = fread(prior, 1, sizeof prior, f); fclose(f);
    assert(n == old_n + 4 && encoded[4] == 2 && prior[4] == 1);
    n = old_n;
    kc_item temp = items[127]; items[127] = items[128]; items[128] = temp;
    uint32_t seq; int lost;
    assert(ku_save_decode(&dst, &scratch, prior, n, &seq, 0, -1, &lost));
    assert(seq == 77 && dst.store.cooked_count == 128);
    assert(dst.store.cooked[127].recipe == 128 && dst.sessions[128].visited == 1);
    assert(ku_recipe_bit(dst.favorites, 128) && ku_recipe_bit(dst.wanted, 128));
    assert(!ku_recipe_bit(dst.favorites, 127) && !ku_recipe_bit(dst.wanted, 127));
    temp = items[127]; items[127] = items[128]; items[128] = temp;
    memset(&src, 0, sizeof src); memset(&dst, 0, sizeof dst); memset(&scratch, 0, sizeof scratch);
    puts("PASS: old 128 KST1 exact bytes and stable-ID migration to index 128");
}
static void storage(void) {
    setup();
    for (unsigned i = 0; i < KR_MAX_RECIPES; ++i) {
        ku_recipe_flip(src.favorites, i); ku_recipe_flip(src.wanted, i);
        src.sessions[i] = (ks_session){1,0,0}; src.timer_steps[i] = 1;
        src.store.cooked[i] = (ks_cooked){i, i + 1, 20000};
    }
    src.store.cooked_count = KR_MAX_RECIPES;
    src.store.history_count = KS_HISTORY_SIZE;
    for (unsigned i = 0; i < KS_HISTORY_SIZE; ++i) src.store.history[i] = (ks_record){i, 20000};
    src.shopping_count = KU_SHOPPING_CAPACITY;
    for (unsigned i = 0; i < KU_SHOPPING_CAPACITY; ++i)
        src.shopping[i] = (ku_shopping){i + 1, 100, 0, 0, 0};
    for (unsigned i = 0; i < km_ingredient_count; ++i) {
        src.excluded |= UINT64_C(1) << i; src.fridge |= UINT64_C(1) << i;
    }
    for (unsigned i = 0; i < 3; ++i) {
        uint32_t id; assert(kt_add(&src.timers, 60, 0, &id));
        src.timer_ids[i] = id; src.timer_recipe[i] = i; src.timer_step[i] = 0;
    }
    src.saved_count = 4; src.has_saved = 1; src.saved_result[0] = 63; src.saved_result[1] = 64; src.saved_result[2] = 127; src.saved_result[3] = 128;
    /* A genuinely full state exceeds the legacy 8192-byte buffer. */
    assert(!ku_save_encode(&src, encoded, 8192, 77, 0, -1));
    size_t n = ku_save_encode(&src, encoded, sizeof(encoded), 77, 0, -1);
    assert(n > 8192 && n < KU_SAVE_MAX); uint32_t seq; int lost;
    assert(ku_save_decode(&dst, &scratch, encoded, n, &seq, 0, -1, &lost));
    assert(seq == 77 && dst.store.cooked_count == KR_MAX_RECIPES);
    assert(dst.store.history_count == KS_HISTORY_SIZE && dst.shopping_count == KU_SHOPPING_CAPACITY);
    assert(dst.excluded == src.excluded && dst.fridge == src.fridge && dst.timers.count == 3 && lost);
    for (unsigned i = 0; i < KR_MAX_RECIPES; ++i) {
        assert(ku_recipe_bit(dst.favorites, i) && ku_recipe_bit(dst.wanted, i));
        assert(dst.sessions[i].visited == 1 && dst.timer_steps[i] == 1);
        assert(dst.store.cooked[i].recipe == i && dst.store.cooked[i].count == i + 1);
    }
    assert(dst.saved_result[0] == 63 && dst.saved_result[1] == 64 && dst.saved_result[2] == 127 && dst.saved_result[3] == 128);
    /* The same KST1 bytes remap through stable IDs across both word boundaries. */
    kc_item tmp = items[63]; items[63] = items[64]; items[64] = tmp;
    tmp = items[127]; items[127] = items[128]; items[128] = tmp;
    assert(ku_save_decode(&dst, &scratch, encoded, n, &seq, 0, -1, &lost));
    assert(dst.saved_result[0] == 64 && dst.saved_result[1] == 63 && dst.saved_result[2] == 128 && dst.saved_result[3] == 127);
    assert(dst.store.cooked[63].recipe == 64 && dst.store.cooked[64].recipe == 63 && dst.store.cooked[127].recipe == 128 && dst.store.cooked[128].recipe == 127);
    printf("PASS: 192 KST1 entries, boundary IDs, remap; maximum encoded %zu bytes\n", n);
}
static void recommend(void) {
    kr_recipe recipes[KR_MAX_RECIPES];
    for (unsigned i = 0; i < KR_MAX_RECIPES; ++i)
        recipes[i] = (kr_recipe){UINT64_C(1) << (i % 64), UINT64_C(1) << (i % 64), 1, 100, KR_MEAT|KR_VEG|KR_SOUP, i % 7, 0};
    unsigned visited[KR_MAX_RECIPES] = {0};
    for (unsigned seed = 0; seed < 50; ++seed) {
        kr_result x = kitchen_recommend(recipes, KR_MAX_RECIPES, 0, 3, 0, seed, NULL, 0);
        assert(x.status == KR_OK && x.count == 4 && kr_test_visits < 400000);
        for (unsigned j = 0; j < 4; ++j) {
            assert(x.indices[j] < KR_MAX_RECIPES); visited[x.indices[j]]++;
            for (unsigned k = 0; k < j; ++k) assert(x.indices[j] != x.indices[k]);
        }
        kr_result y = kitchen_recommend(recipes, KR_MAX_RECIPES, 0, 3, 0, seed, NULL, 0);
        assert(x.status == y.status && x.count == y.count && x.notes == y.notes &&
               !memcmp(x.indices, y.indices, sizeof(x.indices)) && kr_test_visits < 400000);
    }
    unsigned diversity = 0;
    for (unsigned i = 0; i < KR_MAX_RECIPES; ++i) diversity += !!visited[i];
    assert(diversity > 40);
    for (unsigned i=0; i<KR_MAX_RECIPES; ++i) recipes[i].ingredients |= 1;
    kr_result x = kitchen_recommend(recipes, KR_MAX_RECIPES, 1, 3, 0, 1, NULL, 0);
    assert(x.status == KR_NO_ALLOWED);
    /* Adversarial all-equal main/method and total time forces the final relaxation.
     * The explicit visit count proves the search has an input-independent bound. */
    for (unsigned i=0; i<KR_MAX_RECIPES; ++i) {
        recipes[i].ingredients = recipes[i].main_ingredients = 1;
        recipes[i].courses = KR_MEAT|KR_VEG|KR_SOUP;
        recipes[i].method = 0; recipes[i].minutes = 60; recipes[i].repeated_main = 1;
    }
    x = kitchen_recommend(recipes, KR_MAX_RECIPES, 0, 3, 15, 1, NULL, 0);
    assert(x.status == KR_OK && x.notes == 7 && kr_test_visits < 400000);
    size_t adversarial_visits = kr_test_visits;
    /* Candidate pools must never discard the only feasible distinct role combination. */
    for (unsigned i=0; i<KR_MAX_RECIPES; ++i) {
        recipes[i].ingredients = recipes[i].main_ingredients = 0;
        recipes[i].courses = i < 2 ? KR_MEAT : i == 2 ? KR_VEG : i == 3 ? KR_SOUP : 0;
    }
    x = kitchen_recommend(recipes, KR_MAX_RECIPES, 0, 3, 0, 1, NULL, 0);
    assert(x.status == KR_OK && x.count == 4 && kr_test_visits < 400000);
    printf("PASS: 192-recipe bounded deterministic draw, hard exclusions and role edge cases; diversity %u; adversarial visits %zu\n", diversity, adversarial_visits);
}
int main(void) { legacy_128(); storage(); recommend(); }
