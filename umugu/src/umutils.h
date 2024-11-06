#ifndef __UMUGU_UTILS_H__
#define __UMUGU_UTILS_H__

#include "umugu/umugu.h"

#include <assert.h>

/* UMUGU UTILITIES */

typedef int64_t umu_nanosec;
umu_nanosec umu_time_now(void);
static inline float umu_time_sec(umu_nanosec ns) { return ns / 1000000000.0f; }
static inline int64_t umu_time_ms(umu_nanosec ns) { return ns / 1000000; }
static inline int64_t umu_time_micro(umu_nanosec ns) { return ns / 1000; }
static inline umu_nanosec umu_time_elapsed(umu_nanosec ns) {
    return umu_time_now() - ns;
}

/**
 * Note frequencies lookup table.
 * The values correspond to the notes C4 to B4.
 * @see umu_note_freq
 */
static const float umu_note_freq_lut[] = {261.63f, 277.18f, 293.66f, 311.13f,
                                          329.63f, 349.23f, 369.99f, 392.0f,
                                          415.3f,  440.0f,  466.16f, 493.88f};

static inline float umu_note_freq(int abs_note_idx) {
    const float transpose_oct_lut[] = {0.03125f, 0.0625f, 0.125f, 0.25f,
                                       0.5f,     1.0f,    2.0f,   4.0f,
                                       8.0f,     16.0f,   32.0f};
    int oct = abs_note_idx / 12;
    int note = abs_note_idx % 12;
    return umu_note_freq_lut[note] * transpose_oct_lut[oct];
}

static inline int umu_type_size(int type) {
    switch (type) {
    case UMUGU_TYPE_INT8:
    case UMUGU_TYPE_UINT8:
    case UMUGU_TYPE_TEXT:
    case UMUGU_TYPE_BOOL:
        return 1;
    case UMUGU_TYPE_INT16:
        return 2;
    case UMUGU_TYPE_INT32:
    case UMUGU_TYPE_FLOAT:
        return 4;
    case UMUGU_TYPE_INT64:
        return 8;
    default:
        assert(0 && "Please, implement this UMUGU_TYPE_ size getter.");
        return 0;
    }
}

/**
 * Get the amount of bytes between a sample value and the next of the same
 * channel.
 * @note The returned bytes are from start to start, so for non interleaved
 * (tightly packed) values it will still return the size of the sample type
 * instead of zero.
 * @return The jump in bytes from the start of the current value to the start
 * of the next value of the same channel.
 */
static inline int umu_generic_signal_stride(umugu_generic_signal *signal) {
    int size = umu_type_size(signal->type);
    return signal->flags & UMUGU_SIGNAL_INTERLEAVED ? size * signal->channels
                                                    : size;
}

/**
 * @brief The amount of bytes between the start of a channel's sample data and
 * the start of the next channel's sample data.
 * @note For interleaved signals this value is the same as the sample type size.
 * @example For obtaining the fifth channel's offset (index 4), do:
 * umutils_signal_offset(my_signal) * 4.
 * @return The jump in bytes from the first sample value of a channel to the
 * first sample value of the next channel.
 */
static inline int umu_generic_signal_offset(umugu_generic_signal *signal) {
    int size = umu_type_size(signal->type);
    return signal->flags & UMUGU_SIGNAL_INTERLEAVED ? size
                                                    : size * signal->count;
}

static inline void *umu_generic_signal_sample(umugu_generic_signal *signal,
                                              int frame, int channel) {
    int stride = umu_generic_signal_stride(signal) * frame;
    int offset = umu_generic_signal_offset(signal) * channel;
    return (char *)signal->sample_data + offset + stride;
}

static inline umugu_sample *umu_signal_get_channel(umugu_signal signal,
                                                   int channel) {
    assert(signal.samples && (signal.count > 0) && signal.channels >= 0);
    channel *= ((channel >= 0) && (channel < signal.channels));
    return signal.samples + signal.count * channel;
}

// TODO: Generate a binary file for mmaping instead of a .c
// Exports the wave look-up table to a c file.
// Remember to call umugu__fill_wave_lut before exporting, if
// you don't, the generated file will be exactly the same as before.
void umu_gen_waveform_file(const char *filename);
// Recalculate the wave look-up table values.
void umu_fill_waveform_lut(void);

#endif /* __UMUGU_UTILS_H__ */
