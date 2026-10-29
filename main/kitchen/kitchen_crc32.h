#ifndef KITCHEN_CRC32_H
#define KITCHEN_CRC32_H
#include <stddef.h>
#include <stdint.h>

/* IEEE CRC-32, with zlib-compatible seed/finalization. The small read-only
 * nibble table replaces eight polynomial iterations per byte. No heap/RTOS. */
static inline uint32_t kcrc32_update(uint32_t seed, const uint8_t* data, size_t size) {
    static const uint32_t table[16] = {
        0x00000000u, 0x1db71064u, 0x3b6e20c8u, 0x26d930acu,
        0x76dc4190u, 0x6b6b51f4u, 0x4db26158u, 0x5005713cu,
        0xedb88320u, 0xf00f9344u, 0xd6d6a3e8u, 0xcb61b38cu,
        0x9b64c2b0u, 0x86d3d2d4u, 0xa00ae278u, 0xbdbdf21cu,
    };
    uint32_t crc = ~seed;
    while (size--) {
        crc ^= *data++;
        crc = (crc >> 4) ^ table[crc & 15u];
        crc = (crc >> 4) ^ table[crc & 15u];
    }
    return ~crc;
}
static inline uint32_t kcrc32(const uint8_t* data, size_t size) {
    return kcrc32_update(0, data, size);
}
#endif
