#include "kitchen_journal.h"
#include <string.h>
kj_result kj_load(kj_journal* j, const kj_io* io, ku_ui* ui, ku_ui* scratch, uint8_t* buffer,
                  int64_t mono, int64_t wall, int* lost) {
    if (!j || !io || !io->read || !io->write || !ui || !scratch || !buffer)
        return KJ_IO_ERROR;
    *j = (kj_journal){0, -1, 0};
    uint32_t sequences[2] = {0};
    uint16_t versions[2] = {0};
    int good[2] = {0}, found[2] = {0}, present = 0;
    for (unsigned slot = 0; slot < 2; ++slot) {
        size_t length = 0;
        int r = io->read(io->context, slot, buffer, KU_SAVE_MAX, &length);
        if (r < 0)
            return KJ_IO_ERROR;
        if (!r)
            continue;
        present = 1;
        found[slot] = 1;
        good[slot] = ku_save_header(buffer, length, &versions[slot], &sequences[slot], NULL);
    }
    if (!good[0] && !good[1]) {
        if (present)
            return KJ_CORRUPT;
        j->ready = 1;
        if (lost)
            *lost = 0;
        return KJ_EMPTY;
    }
    unsigned slot = good[0] && good[1] ? ((int32_t)(sequences[1] - sequences[0]) > 0 ? 1 : 0)
                    : good[1]          ? 1
                                       : 0;
    if (versions[slot] != 1)
        return KJ_INCOMPATIBLE;
    size_t length = 0;
    if (io->read(io->context, slot, buffer, KU_SAVE_MAX, &length) != 1)
        return KJ_IO_ERROR;
    uint32_t decoded = 0;
    if (!ku_save_decode(ui, scratch, buffer, length, &decoded, mono, wall, lost) ||
        decoded != sequences[slot])
        return KJ_INCOMPATIBLE;
    j->active = (int)slot;
    j->sequence = decoded;
    j->ready = 1;
    return (found[0] && !good[0]) || (found[1] && !good[1]) ? KJ_RECOVERED : KJ_OK;
}
kj_result kj_store(kj_journal* j, const kj_io* io, const ku_ui* ui, uint8_t* buffer, int64_t mono,
                   int64_t wall) {
    if (!j || !j->ready || !io || !io->read || !io->write || !ui || !buffer)
        return KJ_IO_ERROR;
    unsigned slot = j->active == 0 ? 1 : 0;
    uint32_t seq = j->sequence + 1;
    size_t length = ku_save_encode(ui, buffer, KU_SAVE_MAX, seq, mono, wall);
    if (!length)
        return KJ_ENCODE_ERROR;
    uint32_t expected;
    if (!ku_save_header(buffer, length, NULL, NULL, &expected))
        return KJ_ENCODE_ERROR;
    if (!io->write(io->context, slot, buffer, length))
        return KJ_IO_ERROR;
    size_t read_length = 0;
    uint16_t version;
    uint32_t actual_seq, actual_crc;
    if (io->read(io->context, slot, buffer, KU_SAVE_MAX, &read_length) != 1 ||
        read_length != length ||
        !ku_save_header(buffer, read_length, &version, &actual_seq, &actual_crc) || version != 1 ||
        actual_seq != seq || actual_crc != expected)
        return KJ_IO_ERROR;
    j->active = (int)slot;
    j->sequence = seq;
    return KJ_OK;
}
