#ifndef KITCHEN_KRC_H
#define KITCHEN_KRC_H
#include <stddef.h>
#include <stdint.h>
#define KRC_MAX_BODY 2048
typedef struct {
    const uint8_t* data;
    size_t size;
    uint32_t count, dictionary_offset, dictionary_length, bitmap_offset, bitmap_width, body_offset;
} krc_pack;
typedef struct {
    uint32_t id_hash;
    uint16_t tags;
    uint8_t courses, minutes, difficulty;
    const uint8_t* main_bitmap;
} krc_index;
/* Checks container, CRC, sections and body extents, without parsing JSON. */
int krc_open(krc_pack* out, const uint8_t* data, size_t size);
int krc_index_at(const krc_pack* pack, uint32_t index, krc_index* out);
/* Writes JSON bytes without a terminator, returns decoded length or -1. */
int krc_read_body(const krc_pack* pack, uint32_t index, uint8_t* output, size_t capacity);
#endif
