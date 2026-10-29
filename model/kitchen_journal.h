#ifndef KITCHEN_JOURNAL_H
#define KITCHEN_JOURNAL_H
#include "kitchen_save.h"
typedef struct {
    void* context;
    /* read: 1=found, 0=absent, -1=I/O error. Never truncate an existing blob. */
    int (*read)(void* context, unsigned slot, uint8_t* buffer, size_t capacity, size_t* length);
    /* Atomic replace of a single slot, including durable commit: 1=success. */
    int (*write)(void* context, unsigned slot, const uint8_t* buffer, size_t length);
} kj_io;
typedef enum {
    KJ_OK,
    KJ_EMPTY,
    KJ_IO_ERROR,
    KJ_CORRUPT,
    KJ_INCOMPATIBLE,
    KJ_ENCODE_ERROR,
    KJ_RECOVERED
} kj_result;
typedef struct {
    uint32_t sequence;
    int active, ready;
} kj_journal;
/* Caller supplies an 8192-byte buffer and an independent scratch UI. */
kj_result kj_load(kj_journal* journal, const kj_io* io, ku_ui* ui, ku_ui* scratch, uint8_t* buffer,
                  int64_t mono, int64_t wall, int* clock_lost);
kj_result kj_store(kj_journal* journal, const kj_io* io, const ku_ui* ui, uint8_t* buffer,
                   int64_t mono, int64_t wall);
#endif
