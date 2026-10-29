#ifndef NABO_KITCHEN_MIXER_H
#define NABO_KITCHEN_MIXER_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {
    const uint8_t* voice;
    size_t voice_bytes, voice_offset;
    uint32_t tone_left, phase1, phase2;
    bool voice_active;
    uint32_t voice_starts, voice_samples, voice_nonzero_samples;
} nk_mixer;
/* Consume a pending startup greeting once at its deadline. An isolated test
 * cancels it without ever producing audio. Screen transitions do not matter. */
bool nk_boot_voice_due(bool* pending, int64_t now, int64_t deadline, bool test_mode);
void nk_mixer_init(nk_mixer* mixer, const uint8_t* voice, size_t bytes);
void nk_mixer_fill(nk_mixer* mixer, int16_t* samples, size_t count, bool reset, bool start_voice,
                   bool start_alarm, unsigned volume);
#endif
