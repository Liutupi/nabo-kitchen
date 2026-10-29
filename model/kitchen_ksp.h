#ifndef KITCHEN_KSP_H
#define KITCHEN_KSP_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
    const uint8_t* data;
    size_t size;
    uint32_t groups, count, index_offset, strings_offset, payload_offset;
} ksp_pack;
typedef struct {
    uint16_t frame_width, frame_height, x, y, width, height, palette_count, codec;
    const uint8_t* palette;
    uint32_t offset, length, crc;
} ksp_sprite;
/* Each callback gets a cropped row of palette indices and its full-frame x/y. */
typedef int (*ksp_row_fn)(void* context, unsigned x, unsigned y, const uint8_t* indices,
                          unsigned width);
int ksp_open(ksp_pack* out, const uint8_t* data, size_t size);
int ksp_find(const ksp_pack* pack, const char* id, ksp_sprite* out);
/* row buffer needs sprite->width bytes; validates payload CRC and palette indices. */
int ksp_read_rows(const ksp_pack* pack, const ksp_sprite* sprite, uint8_t* row, size_t capacity,
                  ksp_row_fn send, void* context);
#endif
