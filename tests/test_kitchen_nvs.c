/* Actual journal, KST1 codec and NVS engine; all records are synthetic. */
#include "kitchen_metadata.h"
#include "nabo_kitchen_store.h"
#include "nvs.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test_nvs_reset(void);
void test_nvs_reboot(void);
void test_nvs_snapshot(void);
void test_nvs_restore(void);
void test_nvs_cut_after(int n);
int test_nvs_mutations(void);
static kc_item items[KR_MAX_RECIPES];
static kc_catalog catalog = {items, KR_MAX_RECIPES, KR_MAX_RECIPES};
static ku_ui src, dst, scratch;
static uint8_t buffer[KU_SAVE_MAX];
static uint8_t other[2048];

static void setup(void) {
    memset(&src, 0, sizeof src);
    src.catalog = dst.catalog = &catalog;
    src.conditions = src.saved_conditions = (ku_conditions){2, 30, 0};
    src.month = 10;
    src.difficulty = 2;
    src.reroll_limit = 3;
    src.sound = 1;
    src.volume = 50;
    kt_init(&src.timers);
    for (unsigned i = 0; i < KR_MAX_RECIPES; ++i) {
        items[i].hash = 0x9e3779b9u * (i + 1);
        ku_recipe_flip(src.favorites, i);
        ku_recipe_flip(src.wanted, i);
        src.sessions[i] = (ks_session){1, 0, 0};
        src.timer_steps[i] = 1;
        src.store.cooked[i] = (ks_cooked){i, i + 1, 20000};
    }
    src.store.cooked_count = KR_MAX_RECIPES;
    src.store.history_count = KS_HISTORY_SIZE;
    for (unsigned i = 0; i < KS_HISTORY_SIZE; ++i)
        src.store.history[i] = (ks_record){i, 20000};
    src.shopping_count = KU_SHOPPING_CAPACITY;
    for (unsigned i = 0; i < KU_SHOPPING_CAPACITY; ++i)
        src.shopping[i] = (ku_shopping){i + 1, 100, 0, 0, 0};
    for (unsigned i = 0; i < km_ingredient_count; ++i) {
        src.excluded |= UINT64_C(1) << i;
        src.fridge |= UINT64_C(1) << i;
    }
    for (unsigned i = 0; i < 3; ++i) {
        uint32_t id;
        assert(kt_add(&src.timers, 60, 0, &id));
        src.timer_ids[i] = id;
        src.timer_recipe[i] = i;
    }
    src.saved_count = 4;
    src.has_saved = 1;
    for (unsigned i = 0; i < 4; ++i)
        src.saved_result[i] = i;
    for (unsigned i = 0; i < sizeof other; ++i)
        other[i] = (uint8_t)(i * 37);
}
static void other_write(void) {
    nvs_handle_t h;
    assert(nvs_open("other_settings", NVS_READWRITE, &h) == ESP_OK);
    assert(nvs_set_blob(h, "keep", other, sizeof other) == ESP_OK);
    assert(nvs_commit(h) == ESP_OK);
    nvs_close(h);
}
static void other_check(void) {
    nvs_handle_t h;
    uint8_t actual[sizeof other];
    size_t n = sizeof actual;
    assert(nvs_open("other_settings", NVS_READONLY, &h) == ESP_OK);
    assert(nvs_get_blob(h, "keep", actual, &n) == ESP_OK);
    assert(n == sizeof other && !memcmp(actual, other, n));
    nvs_close(h);
}
int main(void) {
    setup();
    test_nvs_reset();
    other_write();
    kj_io io = nabo_kitchen_store_io();
    kj_journal j;
    assert(kj_load(&j, &io, &dst, &scratch, buffer, 0, -1, NULL) == KJ_EMPTY);
    for (unsigned i = 1; i <= 100; ++i) {
        src.daily_seed = i;
        kj_result r = kj_store(&j, &io, &src, buffer, 0, -1);
        if (r != KJ_OK)
            fprintf(stderr, "save %u failed: %d\n", i, r);
        assert(r == KJ_OK);
        test_nvs_reboot();
        assert(kj_load(&j, &io, &dst, &scratch, buffer, 0, -1, NULL) == KJ_OK);
        assert(dst.daily_seed == i && j.sequence == i);
        other_check();
    }
    test_nvs_snapshot();
    src.daily_seed = 101;
    test_nvs_cut_after(-1);
    assert(kj_store(&j, &io, &src, buffer, 0, -1) == KJ_OK);
    int writes = test_nvs_mutations();
    for (int cut = 0; cut <= writes; ++cut) {
        test_nvs_restore();
        assert(kj_load(&j, &io, &dst, &scratch, buffer, 0, -1, NULL) == KJ_OK);
        test_nvs_cut_after(cut);
        (void)kj_store(&j, &io, &src, buffer, 0, -1);
        test_nvs_reboot();
        kj_result r = kj_load(&j, &io, &dst, &scratch, buffer, 0, -1, NULL);
        assert(r == KJ_OK || r == KJ_RECOVERED);
        assert(dst.daily_seed == 100 || dst.daily_seed == 101);
        assert(dst.store.cooked_count == KR_MAX_RECIPES &&
               dst.shopping_count == KU_SHOPPING_CAPACITY);
        other_check();
    }
    printf("PASS: 100 maximum-state saves in 24 KiB NVS with 2 KiB unrelated settings; %d "
           "power-cut boundaries\n",
           writes + 1);
    /* An unrelated namespace may fill the partition. Do not destroy the active
     * save or those settings when even reclamation of the stale slot cannot fit. */
    test_nvs_reset();
    other_write();
    assert(kj_load(&j, &io, &dst, &scratch, buffer, 0, -1, NULL) == KJ_EMPTY);
    src.daily_seed = 201;
    assert(kj_store(&j, &io, &src, buffer, 0, -1) == KJ_OK);
    nvs_handle_t h;
    static uint8_t occupied[8192];
    memset(occupied, 0x5a, sizeof occupied);
    assert(nvs_open("other_settings", NVS_READWRITE, &h) == ESP_OK);
    assert(nvs_set_blob(h, "occupied", occupied, sizeof occupied) == ESP_OK);
    assert(nvs_commit(h) == ESP_OK);
    nvs_close(h);
    src.daily_seed = 202;
    assert(kj_store(&j, &io, &src, buffer, 0, -1) == KJ_IO_ERROR);
    assert(j.sequence == 1 && j.active == 0);
    test_nvs_reboot();
    assert(kj_load(&j, &io, &dst, &scratch, buffer, 0, -1, NULL) == KJ_OK);
    assert(dst.daily_seed == 201);
    other_check();
    assert(nvs_open("other_settings", NVS_READONLY, &h) == ESP_OK);
    size_t n = sizeof buffer;
    assert(nvs_get_blob(h, "occupied", buffer, &n) == ESP_OK && n == sizeof occupied);
    assert(!memcmp(buffer, occupied, n));
    nvs_close(h);
    puts("PASS: completely full NVS reports an error and preserves the active save and unrelated "
         "namespace");
    return 0;
}
