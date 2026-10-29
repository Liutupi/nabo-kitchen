#ifndef KITCHEN_SAVE_H
#define KITCHEN_SAVE_H
#include "kitchen_ui.h"
#define KU_SAVE_MAX 8192
/* Stable IDs, little-endian format, CRC, and a caller-managed sequence.
 * wall_ms is epoch milliseconds, or -1 when unavailable. Timer remaining is
 * captured using monotonic_ms. The caller owns atomic two-slot storage. */
size_t ku_save_encode(const ku_ui* ui, uint8_t* buffer, size_t capacity, uint32_t sequence,
                      int64_t monotonic_ms, int64_t wall_ms);
/* No state changes on failure. Running timers pause when wall time is unknown;
 * clock_lost is set so the UI can explicitly notify the user. */
int ku_save_decode(ku_ui* ui, ku_ui* scratch, const uint8_t* buffer, size_t length,
                   uint32_t* sequence, int64_t monotonic_ms, int64_t wall_ms, int* clock_lost);
/* Header and CRC check without interpreting the versioned body. */
int ku_save_header(const uint8_t* buffer, size_t length, uint16_t* version, uint32_t* sequence,
                   uint32_t* crc);
#endif
