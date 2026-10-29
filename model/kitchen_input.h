#ifndef KITCHEN_INPUT_H
#define KITCHEN_INPUT_H
#include <stdint.h>
typedef enum { KI_UP, KI_DOWN, KI_OK, KI_KEYS } ki_key;
typedef enum { KI_SHORT, KI_DOUBLE, KI_LONG } ki_kind;
typedef void (*ki_dispatch)(void* context, ki_key key, ki_kind kind);
typedef struct {
    int64_t pressed_at[KI_KEYS], pending_at, last_now;
    uint32_t sequence[KI_KEYS], pending_sequence, next_sequence;
    uint8_t held[KI_KEYS], long_sent[KI_KEYS], pending, initialized;
} ki_input;
void ki_init(ki_input* input);
/* Poll before events at the same timestamp; clocks must be nondecreasing.
 * Dispatch must not block. Return 0 for invalid clock/key or overflow. */
int ki_poll(ki_input* input, int64_t now, ki_dispatch send, void* context);
int ki_press(ki_input* input, ki_key key, int64_t now, ki_dispatch send, void* context);
int ki_release(ki_input* input, ki_key key, int cancel, int64_t now, ki_dispatch send,
               void* context);
int ki_blur(ki_input* input, int64_t now, ki_dispatch send, void* context);
#endif
