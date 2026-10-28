#include "kitchen_timer.h"
#include <limits.h>
#include <string.h>

static kt_timer *find(kt_service *s, uint32_t id) {
    for (uint32_t i = 0; i < s->count; ++i)
        if (s->items[i].id == id) return &s->items[i];
    return 0;
}
void kt_init(kt_service *s) { memset(s, 0, sizeof(*s)); s->next_id = 1; }
int kt_add(kt_service *s, uint32_t seconds, int64_t now, uint32_t *id) {
    int64_t duration = (int64_t)seconds * 1000;
    if (!seconds || s->count >= KITCHEN_TIMER_CAPACITY || !s->next_id ||
        now > INT64_MAX - duration) return 0;
    kt_timer *t = &s->items[s->count++];
    *t = (kt_timer){s->next_id++, duration, duration, now + duration, KT_RUNNING, 0};
    if (id) *id = t->id;
    return 1;
}
int64_t kt_remaining(const kt_timer *t, int64_t now) {
    if (t->status != KT_RUNNING) return t->left_ms > 0 ? t->left_ms : 0;
    if (now >= t->deadline_ms) return 0;
    /* Saturate when a caller supplies a time far outside the timer's clock domain. */
    if (now < 0 && t->deadline_ms > INT64_MAX + now) return INT64_MAX;
    return t->deadline_ms - now;
}
int kt_tick(kt_service *s, int64_t now) {
    int changed = 0;
    for (uint32_t i = 0; i < s->count; ++i) {
        kt_timer *t = &s->items[i];
        if (t->status == KT_RUNNING && kt_remaining(t, now) == 0) {
            t->status = KT_DONE; t->left_ms = 0; changed = 1;
        }
    }
    return changed;
}
void kt_toggle(kt_service *s, uint32_t id, int64_t now) {
    kt_timer *t = find(s, id);
    if (!t) return;
    if (t->status == KT_RUNNING) { t->left_ms = kt_remaining(t, now); t->status = KT_PAUSED; }
    else if (t->status == KT_PAUSED && now <= INT64_MAX - t->left_ms) {
        t->deadline_ms = now + t->left_ms; t->status = KT_RUNNING;
    }
}
void kt_cancel(kt_service *s, uint32_t id) {
    for (uint32_t i = 0; i < s->count; ++i) if (s->items[i].id == id) {
        memmove(&s->items[i], &s->items[i + 1], (s->count - i - 1) * sizeof(s->items[0]));
        --s->count; return;
    }
}
uint32_t kt_alarm(kt_service *s, int64_t now) {
    kt_tick(s, now);
    for (uint32_t i = 0; i < s->count; ++i)
        if (s->items[i].status == KT_DONE && !s->items[i].acknowledged) return s->items[i].id;
    return 0;
}
void kt_acknowledge(kt_service *s, uint32_t id) { kt_timer *t = find(s, id); if (t) t->acknowledged = 1; }
