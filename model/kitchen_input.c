#include "kitchen_input.h"
#include <limits.h>
#include <string.h>
#define LONG_MS 650
#define DOUBLE_MS 280

void ki_init(ki_input* s) { memset(s, 0, sizeof(*s)); }
static int clock_valid(ki_input* s, int64_t now) {
    if (now < 0 || now > INT64_MAX - LONG_MS || (s->initialized && now < s->last_now))
        return 0;
    s->last_now = now;
    s->initialized = 1;
    return 1;
}
int ki_poll(ki_input* s, int64_t now, ki_dispatch send, void* ctx) {
    if (!s || !send || !clock_valid(s, now))
        return 0;
    for (;;) {
        int kind = -1;
        int64_t earliest = INT64_MAX;
        uint32_t seq = UINT32_MAX;
        if (s->pending && s->pending_at + DOUBLE_MS <= now) {
            kind = KI_KEYS;
            earliest = s->pending_at + DOUBLE_MS;
            seq = s->pending_sequence;
        }
        for (int i = 0; i < KI_KEYS; ++i)
            if (s->held[i] && !s->long_sent[i]) {
                int64_t due = s->pressed_at[i] + LONG_MS;
                if (due <= now && (due < earliest || (due == earliest && s->sequence[i] < seq))) {
                    kind = i;
                    earliest = due;
                    seq = s->sequence[i];
                }
            }
        if (kind < 0)
            break;
        if (kind == KI_KEYS) {
            s->pending = 0;
            send(ctx, KI_UP, KI_SHORT);
        } else {
            s->long_sent[kind] = 1;
            s->pending = 0;
            send(ctx, (ki_key)kind, KI_LONG);
        }
    }
    return 1;
}
int ki_press(ki_input* s, ki_key key, int64_t now, ki_dispatch send, void* ctx) {
    if (key < KI_UP || key >= KI_KEYS || !ki_poll(s, now, send, ctx))
        return 0;
    if (s->held[key])
        return 1;
    if (s->next_sequence == UINT32_MAX)
        return 0;
    if (key != KI_UP && s->pending) {
        s->pending = 0;
        send(ctx, KI_UP, KI_SHORT);
    }
    s->held[key] = 1;
    s->long_sent[key] = 0;
    s->pressed_at[key] = now;
    s->sequence[key] = ++s->next_sequence;
    return 1;
}
int ki_release(ki_input* s, ki_key key, int cancel, int64_t now, ki_dispatch send, void* ctx) {
    if (key < KI_UP || key >= KI_KEYS || !ki_poll(s, now, send, ctx))
        return 0;
    if (!s->held[key])
        return 1;
    int was_long = s->long_sent[key];
    s->held[key] = 0;
    if (cancel || was_long)
        return 1;
    if (key == KI_UP) {
        if (s->pending && now - s->pending_at <= DOUBLE_MS) {
            s->pending = 0;
            send(ctx, KI_UP, KI_DOUBLE);
        } else {
            if (s->next_sequence == UINT32_MAX)
                return 0;
            s->pending = 1;
            s->pending_at = now;
            s->pending_sequence = ++s->next_sequence;
        }
    } else {
        s->pending = 0;
        send(ctx, key, KI_SHORT);
    }
    return 1;
}
int ki_blur(ki_input* s, int64_t now, ki_dispatch send, void* ctx) {
    if (!ki_poll(s, now, send, ctx))
        return 0;
    memset(s->held, 0, sizeof(s->held));
    s->pending = 0;
    return 1;
}
