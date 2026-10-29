#include "nabo_kitchen_mixer.h"
#include <limits.h>
#include <string.h>
bool nk_boot_voice_due(bool* pending, int64_t now, int64_t deadline, bool test_mode) {
    if (!pending || !*pending)
        return false;
    if (test_mode) {
        *pending = false;
        return false;
    }
    if (now < deadline)
        return false;
    *pending = false;
    return true;
}
void nk_mixer_init(nk_mixer* m, const uint8_t* voice, size_t bytes) {
    memset(m, 0, sizeof(*m));
    m->voice = voice;
    m->voice_bytes = bytes & ~(size_t)1;
}
void nk_mixer_fill(nk_mixer* m, int16_t* samples, size_t count, bool reset, bool start_voice,
                   bool start_alarm, unsigned volume) {
    if (reset) {
        m->tone_left = 0;
        m->voice_active = false;
    }
    if (start_voice) {
        m->voice_offset = 0;
        m->voice_active = m->voice && m->voice_bytes;
        if (m->voice_active)
            ++m->voice_starts;
    }
    if (start_alarm)
        m->tone_left = 16800;
    if (volume > 100)
        volume = 100;
    for (size_t i = 0; i < count; ++i) {
        int value = samples[i];
        if (m->voice_active && m->voice_offset + 1 < m->voice_bytes) {
            int16_t sample = (int16_t)((uint16_t)m->voice[m->voice_offset] |
                                       (uint16_t)m->voice[m->voice_offset + 1] << 8);
            m->voice_offset += 2;
            int scaled = sample * (int)volume / 100;
            value += scaled;
            ++m->voice_samples;
            if (scaled)
                ++m->voice_nonzero_samples;
        } else
            m->voice_active = false;
        if (m->tone_left) {
            unsigned elapsed = 16800 - m->tone_left, within = elapsed % 5600;
            m->phase1 += UINT32_C(751619277);
            m->phase2 += UINT32_C(885837005);
            if (within < 3200) {
                unsigned ramp = within < 160 ? within : within > 3040 ? 3200 - within : 160;
                int tone = ((m->phase1 >> 31) ? 1700 : -1700) + ((m->phase2 >> 31) ? 1300 : -1300);
                value += tone * (int)ramp * (int)volume / 16000;
            }
            --m->tone_left;
        }
        if (value > INT16_MAX)
            value = INT16_MAX;
        if (value < INT16_MIN)
            value = INT16_MIN;
        samples[i] = (int16_t)value;
    }
}
