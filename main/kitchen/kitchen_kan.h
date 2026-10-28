#ifndef KITCHEN_KAN_H
#define KITCHEN_KAN_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
    const uint8_t* data;
    size_t size;
    uint32_t width, height, fps, palette_count, action_count, index_offset;
} kan_pack;
typedef struct {
    uint16_t frames, loop;
    uint32_t offset, length;
} kan_action;
typedef struct {
    const kan_pack* pack;
    kan_action action;
    uint8_t* frame;
    size_t capacity, position;
    uint16_t next;
    int invalid;
} kan_player;
/* Validated input must stay immutable; errors invalidate the player/frame. */
int kan_open(kan_pack* out, const uint8_t* data, size_t size);
int kan_find(const kan_pack* pack, const char* id, kan_action* out);
int kan_start(kan_player* player, const kan_pack* pack, const kan_action* action, uint8_t* frame,
              size_t capacity);
/* 1=decoded frame, 0=end, -1=corrupt. Restart explicitly for a loop. */
int kan_next(kan_player* player);
#endif
