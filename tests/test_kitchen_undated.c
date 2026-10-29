/* Offline provisional dates must not manufacture calendar-dependent results. */
#include "kitchen_ui.h"
#include "kitchen_save.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

/* The UI dispatches through these readers; use a single untimed step so the
 * test exercises production completion/recommendation without ESP-IDF/cJSON. */
static int timed_step;
int kc_read_step(const krc_pack* pack, unsigned recipe, unsigned step, kc_step* out,
                 unsigned* count) {
    (void)pack;
    if (recipe >= 2 || step != 0 || !out || !count) return 0;
    memset(out, 0, sizeof(*out));
    if (timed_step) out->timer_seconds = 60;
    *count = 1;
    return 1;
}
int kc_read_ingredients(const krc_pack* pack, unsigned recipe, kc_ingredient* out,
                        unsigned capacity, unsigned* count) {
    (void)pack; (void)recipe; (void)out; (void)capacity;
    if (!count) return 0;
    *count = 0;
    return 1;
}

static kc_item items[2];
static kc_catalog catalog = {items, 2, 2};
static krc_pack pack;

static void setup(void) {
    memset(items, 0, sizeof(items));
    for (unsigned i = 0; i < 2; ++i) {
        items[i].recipe = (kr_recipe){UINT64_C(1) << i, UINT64_C(1) << i, 10, 100, KR_VEG, 0, 0};
        items[i].hash = i + 1;
        items[i].difficulty = 1;
        items[i].servings = 1;
    }
    items[0].season_mask = 1u; /* January */
}
static ku_ui fresh(uint32_t seed, unsigned month, int unknown, int recent) {
    ku_ui u;
    assert(ku_init(&u, &catalog, &pack, 21000, month, seed, NULL, 0, NULL, 0));
    assert(u.date_unconfirmed == 0); /* old callers retain confirmed-date semantics */
    u.date_unconfirmed = (uint8_t)unknown;
    if (recent) {
        u.store.history_count = 1;
        u.store.history[0] = (ks_record){0, 21000};
    }
    return u;
}
static unsigned select_one(ku_ui* u) {
    ku_key(u, KI_OK, KI_LONG, 1000);
    assert(u->page == 3 && u->saved_count == 1);
    return u->saved_result[0];
}
static void recommendation_has_no_fake_recency_or_season(void) {
    unsigned confirmed_recent_diff = 0, confirmed_season_diff = 0;
    for (uint32_t seed = 1; seed <= 512; ++seed) {
        ku_ui unknown_a = fresh(seed, 1, 1, 1);
        ku_ui unknown_b = fresh(seed, 12, 1, 0);
        assert(select_one(&unknown_a) == select_one(&unknown_b));
        ku_ui base = fresh(seed, 12, 0, 0);
        ku_ui recent = fresh(seed, 12, 0, 1);
        ku_ui seasonal = fresh(seed, 1, 0, 0);
        unsigned b = select_one(&base);
        confirmed_recent_diff += b != select_one(&recent);
        confirmed_season_diff += b != select_one(&seasonal);
    }
    assert(confirmed_recent_diff && confirmed_season_diff);
}
static void finish(ku_ui* u, unsigned recipe, int32_t day, int unknown) {
    u->date_unconfirmed = (uint8_t)unknown;
    u->day = day;
    u->page = 6;
    u->recipe = (uint16_t)recipe;
    ks_reset_session(&u->sessions[recipe]);
    ku_key(u, KI_OK, KI_SHORT, 2000);
    assert(u->page == 8 && u->sessions[recipe].completed);
}
static void completion_and_transitions(void) {
    ku_ui u = fresh(1, 1, 0, 0);
    finish(&u, 0, 21000, 1);
    assert(u.store.history_count == 1 && u.store.history[0].day == INT32_MIN);
    assert(u.store.cooked_count == 1 && u.store.cooked[0].count == 1);
    assert(!u.store.has_streak && !u.store.streak_days);
    ks_stats s = ks_get_stats(&u.store, INT32_MIN);
    assert(s.unique == 1 && s.total == 1 && !s.today && !s.streak);
    s = ks_get_stats(&u.store, 21000);
    assert(s.total == 1 && !s.today && !s.streak);

    finish(&u, 1, 21000, 0);
    assert(u.store.history[1].day == 21000 && u.store.has_streak && u.store.streak_days == 1);
    s = ks_get_stats(&u.store, 21000);
    assert(s.unique == 2 && s.total == 2 && s.today == 1 && s.streak == 1);
    finish(&u, 0, 21001, 1);
    assert(u.store.history[2].day == INT32_MIN && u.store.streak_last_day == 21000 && u.store.streak_days == 1);
    s = ks_get_stats(&u.store, INT32_MIN);
    assert(s.total == 3 && !s.today && !s.streak);
    s = ks_get_stats(&u.store, 21001);
    assert(s.total == 3 && !s.today && s.streak == 1);
    finish(&u, 0, 21001, 0);
    s = ks_get_stats(&u.store, 21001);
    assert(s.total == 4 && s.today == 1 && s.streak == 2);
    s = ks_get_stats(&u.store, 21003);
    assert(s.total == 4 && s.today == 0 && s.streak == 0);
}
static void sentinel_is_never_a_legacy_day(void) {
    ks_store store;ks_session session;ks_init(&store);ks_reset_session(&session);
    assert(ks_complete_step(&store, &session, 0, 1, INT32_MIN) == KS_COMPLETE);
    assert(!store.has_streak);
    ks_stats s = ks_get_stats(&store, INT32_MIN + 1);
    assert(s.total == 1 && s.streak == 0 && s.today == 0);
    s = ks_get_stats(&store, INT32_MIN);
    assert(s.total == 1 && s.streak == 0 && s.today == 0);
}
static void save_schema_and_runtime_flag(void) {
    ku_ui original = fresh(7, 3, 1, 0);
    finish(&original, 0, 21000, 1);
    uint8_t first[KU_SAVE_MAX], second[KU_SAVE_MAX];
    size_t n = ku_save_encode(&original, first, sizeof(first), 37, 3000, -1);
    assert(n > 0);
    ku_ui different = original;different.date_unconfirmed = 0;
    size_t m = ku_save_encode(&different, second, sizeof(second), 37, 3000, -1);
    assert(m == n && !memcmp(first, second, n)); /* flag adds no persistent field */
    for (unsigned pending = 0; pending <= 1; ++pending) {
        ku_ui dest = fresh(9, 4, (int)pending, 0), scratch;
        uint32_t sequence = 0;int lost = 0;
        assert(ku_save_decode(&dest, &scratch, first, n, &sequence, 3100, -1, &lost));
        assert(sequence == 37 && dest.date_unconfirmed == pending);
        assert(dest.store.history_count == 1 && dest.store.history[0].day == INT32_MIN);
        assert(dest.store.cooked_count == 1 && dest.store.cooked[0].count == 1);
        ks_stats stats = ks_get_stats(&dest.store, 21000);
        assert(stats.total == 1 && stats.today == 0 && stats.streak == 0);
    }
}
static void book_192_boundary(void) {
    static kc_item all[KR_MAX_RECIPES];
    static kc_catalog large = { all, KR_MAX_RECIPES, KR_MAX_RECIPES };
    ku_ui u;
    assert(ku_init(&u, &large, &pack, 21000, 1, 7, NULL, 0, NULL, 0));
    u.page = 12; u.focus = 0;
    ku_key(&u, KI_UP, KI_SHORT, 1000);
    assert(u.focus == KR_MAX_RECIPES);
    ku_key(&u, KI_OK, KI_LONG, 1100);
    assert(ku_recipe_bit(u.favorites, KR_MAX_RECIPES - 1));
    ku_key(&u, KI_OK, KI_SHORT, 1200);
    assert(u.page == 4 && u.recipe == KR_MAX_RECIPES - 1);
}
static void boot_waits_for_short_ok_and_respects_alarm(void) {
    ku_ui u = fresh(1, 1, 1, 0);
    ku_startup(&u, 1000);
    assert(u.page == 0 && u.transition_due == 4600);
    for (unsigned i = 0; i < 4; ++i) {
        int64_t at[] = {1000, 4599, 4600, 60000};
        ku_tick(&u, at[i]);
        assert(u.page == 0);
    }
    /* UP/DOWN, long and double presses never dismiss the opening screen,
     * including a stale 'outside' state; the input clock stays monotonic. */
    u.outside = 1;
    for (int key = KI_UP; key < KI_KEYS; ++key) {
        for (int kind = KI_SHORT; kind <= KI_LONG; ++kind) {
            if (key == KI_OK && kind == KI_SHORT) continue;
            ku_key(&u, (ki_key)key, (ki_kind)kind, 61000);
            assert(u.page == 0);
        }
    }
    ku_key(&u, KI_OK, KI_SHORT, 61001);
    assert(u.page == 1 && !u.outside);
    /* A tap can also skip the opening animation before its deadline. */
    ku_startup(&u, 70000);
    ku_key(&u, KI_OK, KI_SHORT, 70100);
    assert(u.page == 1);

    /* Timer alerts are neither hidden nor acknowledged by the home action:
     * OK must open the active timer first, even during the boot animation. */
    kt_init(&u.timers);
    uint32_t id;
    assert(kt_add(&u.timers, 1, 80000, &id));
    ku_startup(&u, 80000);
    ku_tick(&u, 81000);
    assert(u.page == 0 && kt_alarm(&u.timers, 81000) == id);
    ku_key(&u, KI_DOWN, KI_SHORT, 81001);
    assert(u.page == 0);
    ku_key(&u, KI_OK, KI_SHORT, 81002);
    assert(u.page == 7 && kt_alarm(&u.timers, 81002) == id);
}
static void unlimited_rerolls_keep_advancing_and_roundtrip(void) {
    ku_ui u = fresh(71, 1, 1, 0);
    u.reroll_limit = 1; /* A real old save may still carry a small limit. */
    int64_t now = 1000;
    ku_key(&u, KI_OK, KI_LONG, now);
    assert(u.page == 3);
    ku_tick(&u, now += 1600);
    assert(u.page == 4);
    for (unsigned i = 0; i < 300; ++i) {
        unsigned previous = u.result[0];
        ku_key(&u, KI_DOWN, KI_SHORT, ++now);
        assert(u.page == 3 && u.message == KU_NO_MESSAGE);
        ku_tick(&u, now += 1600);
        assert(u.page == 4 && u.result_count == 1 && u.result[0] != previous);
    }
    assert(u.rerolls_used == UINT8_MAX && u.daily_seed != 71);
    uint8_t data[KU_SAVE_MAX];
    size_t length = ku_save_encode(&u, data, sizeof(data), 9, now, -1);
    ku_ui restored = fresh(1, 1, 1, 0), scratch;
    uint32_t sequence = 0; int lost = 0;
    assert(length && ku_save_decode(&restored, &scratch, data, length, &sequence, now, -1, &lost));
    assert(sequence == 9 && restored.rerolls_used == UINT8_MAX && restored.daily_seed == u.daily_seed);
    restored.page = 4; restored.result_count = restored.saved_count;
    memcpy(restored.result, restored.saved_result, sizeof(restored.result));
    unsigned previous = restored.result[0];
    ku_key(&restored, KI_DOWN, KI_SHORT, ++now);
    assert(restored.page == 3);
    ku_tick(&restored, now += 1600);
    assert(restored.page == 4 && restored.result[0] != previous);
}
static void cancel_only_selected_timer_and_persist(void) {
    ku_ui u = fresh(42, 1, 1, 0);
    uint32_t ids[3];
    for (unsigned i = 0; i < 3; ++i) {
        assert(kt_add(&u.timers, 60 + i, 1000, &ids[i]));
        u.timer_ids[i] = ids[i]; u.timer_recipe[i] = 0; u.timer_step[i] = (uint8_t)i;
    }
    u.timer_steps[0] = 7;
    kt_toggle(&u.timers, ids[1], 1100); /* second is paused */
    u.page = 7; u.focus = 0;
    ku_key(&u, KI_DOWN, KI_LONG, 1200); /* remove running first */
    assert(u.timers.count == 2 && u.timers.items[0].id == ids[1] && u.timers.items[1].id == ids[2]);
    assert(u.timer_ids[0] == 0 && u.timer_steps[0] == 7 && u.dirty);
    uint8_t data[KU_SAVE_MAX];
    size_t length = ku_save_encode(&u, data, sizeof(data), 4, 1200, -1);
    ku_ui restored = fresh(1, 1, 1, 0), scratch;
    uint32_t sequence = 0; int lost = 0;
    assert(length && ku_save_decode(&restored, &scratch, data, length, &sequence, 1300, -1, &lost));
    assert(sequence == 4 && restored.timers.count == 2 && restored.timer_steps[0] == 7);
    assert(restored.timers.items[0].id == ids[1] && restored.timers.items[1].id == ids[2]);
    /* Returning to the canceled step progresses it instead of starting a new timer. */
    timed_step = 1; restored.page = 6; restored.recipe = 0;
    ku_key(&restored, KI_OK, KI_SHORT, 1400);
    assert(restored.timers.count == 2 && restored.page == 8);
    timed_step = 0;
    restored.page = 7; restored.focus = 0;
    ku_key(&restored, KI_DOWN, KI_LONG, 1500); /* remove paused second */
    assert(restored.timers.count == 1 && restored.timers.items[0].id == ids[2]);
    kt_toggle(&restored.timers, ids[2], 1600); /* running again */
    ku_tick(&restored, 100000);
    assert(restored.timers.items[0].status == KT_DONE);
    ku_key(&restored, KI_DOWN, KI_LONG, 100001); /* remove completed third */
    assert(restored.timers.count == 0);
}
static void settings_no_longer_offers_a_reroll_limit(void) {
    ku_ui u = fresh(1, 1, 1, 0);
    u.page = 13; u.focus = 0;
    ku_key(&u, KI_DOWN, KI_SHORT, 1000);
    ku_key(&u, KI_DOWN, KI_SHORT, 1001);
    assert(u.focus == 2);
    ku_key(&u, KI_OK, KI_SHORT, 1002);
    assert(u.settings_mode && u.focus == 0);
    ku_key(&u, KI_UP, KI_DOUBLE, 1003);
    assert(!u.settings_mode && u.focus == 2);
}
static void acknowledged_clock_recovery_survives_restart(void) {
    ku_ui original = fresh(4, 1, 1, 0), restored, scratch;
    original.volume = 37;
    original.sound = 0; /* Recovery must preserve the user's saved mute. */
    uint32_t id;
    assert(kt_add(&original.timers, 120, 1000, &id));
    original.timer_ids[0] = id;
    original.timer_recipe[0] = 0;
    original.timer_step[0] = 0;
    uint8_t buffer[KU_SAVE_MAX];
    size_t size = ku_save_encode(&original, buffer, sizeof(buffer), 1, 2000, -1);
    assert(size);
    restored = fresh(5, 1, 1, 0);
    int lost = 0;
    assert(ku_save_decode(&restored, &scratch, buffer, size, NULL, 20, -1, &lost));
    assert(lost && restored.timers.count == 1);
    assert(restored.timers.items[0].status == KT_PAUSED);
    int64_t left = restored.timers.items[0].left_ms;
    assert(left == 119000);
    restored.message = KU_CLOCK_LOST;
    restored.stack_count = 3;
    restored.dirty = 0; /* Ack must request persistence independently. */
    ku_acknowledge_clock_loss(&restored);
    assert(restored.dirty && restored.page == 7 && !restored.stack_count);
    assert(restored.message == KU_NO_MESSAGE);
    assert(restored.timers.items[0].id == id && restored.volume == 37 && !restored.sound);
    size = ku_save_encode(&restored, buffer, sizeof(buffer), 2, 500, -1);
    assert(size);
    ku_ui rebooted = fresh(6, 1, 1, 0);
    uint32_t seq;
    lost = 1;
    assert(ku_save_decode(&rebooted, &scratch, buffer, size, &seq, 10, -1, &lost));
    assert(!lost && seq == 2 && rebooted.timers.count == 1);
    assert(rebooted.timers.items[0].status == KT_PAUSED);
    assert(rebooted.timers.items[0].left_ms == left && rebooted.timers.items[0].id == id);
    assert(rebooted.volume == 37 && !rebooted.sound);
}
int main(void) {
    book_192_boundary();setup();boot_waits_for_short_ok_and_respects_alarm();
    recommendation_has_no_fake_recency_or_season();completion_and_transitions();
    sentinel_is_never_a_legacy_day();save_schema_and_runtime_flag();
    unlimited_rerolls_keep_advancing_and_roundtrip();
    cancel_only_selected_timer_and_persist();
    settings_no_longer_offers_a_reroll_limit();
    acknowledged_clock_recovery_survives_restart();
    puts("PASS: boot hold and timer priority, unknown-date weighting, completion history, streak and existing save schema");
    return 0;
}
