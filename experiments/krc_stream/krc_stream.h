#ifndef NABO_EXPERIMENT_KRC_STREAM_H
#define NABO_EXPERIMENT_KRC_STREAM_H

#include "kitchen_krc.h"
#include <stddef.h>
#include <stdint.h>

/* Experimental read-only KRC1 cursor. The caller retains the input bytes. */
typedef struct {
  krc_pack pack;
  const uint8_t *next_id, *dictionary_end;
  uint32_t ordinal;
  int failed;
} ksc_cursor;

typedef struct {
  uint32_t ordinal, id_hash;
  uint16_t tags;
  uint8_t courses, minutes, difficulty;
  char id[41];
} ksc_row;

/* A zero course mask and a negative main ingredient mean no such filter.
 * The main ingredient filter is NOT a complete ingredient or allergy filter. */
typedef struct {
  uint8_t any_course;
  int main_ingredient;
} ksc_filter;

int ksc_open(ksc_cursor *cursor, const uint8_t *data, size_t size);
/* Returns 1 for one row, 0 at end, -1 on malformed data. */
int ksc_next(ksc_cursor *cursor, ksc_row *row);
/* Returns 0 on success, -1 on invalid/malformed input. Capacity must be > 0.
 * count is the number produced; finished means end of the entire catalogue. */
int ksc_page(ksc_cursor *cursor, ksc_filter filter, ksc_row *rows,
             size_t capacity, size_t *count, int *finished);
/* Linear scan deliberately checks the full ID, and rejects ambiguous hashes. */
int ksc_find_id(const uint8_t *data, size_t size, const char *id, ksc_row *row);
/* Existing bounded decompressor; returns byte count or -1. */
int ksc_body(const ksc_cursor *cursor, uint32_t ordinal, uint8_t *out,
             size_t cap);

#endif
