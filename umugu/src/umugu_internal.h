#ifndef __UMUGU_INTERNAL_H__
#define __UMUGU_INTERNAL_H__

#include "umugu.h"

void umugu_init_backend(void);
void umugu_close_backend(void);

/* OSCILLOSCOPE */

enum {
    UM__WAVEFORM_SINE = 0,
    UM__WAVEFORM_SAW,
    UM__WAVEFORM_SQUARE,
    UM__WAVEFORM_TRIANGLE,
    UM__WAVEFORM_WHITE_NOISE,
    UM__WAVEFORM_COUNT
};

typedef struct {
    umugu_node node;
    int32_t phase;
    int32_t freq;
    int32_t waveform;
    int32_t sample_capacity;
    umugu_sample *audio_source;
} um__oscope;

umugu_node_fn um__oscope_getfn(umugu_code fn);

/* Look-up table with waveform signals. TODO: mmap binary file instead of compiling .c */
extern const float um__waveform_lut[UM__WAVEFORM_COUNT][UMUGU_SAMPLE_RATE];

// TODO: Generate a binary file for mmaping instead of a .c
// TODO: Merge gen and fill (instead of filling, write to the file).
// Exports the wave look-up table to a c file.
// The file will be formatted and the variable named like the existing osciloscope_lut.c.
// Remember to call umugu__fill_wave_lut before exporting, if you don't the generated file
// will be exactly the same as osciloscope_lut.c.
void um__gen_wave_file(const char *filename);
// Recalculate the wave look-up table values.
void um__fill_wave_lut(void);

/* WAV FILE PLAYER */
typedef struct {
    umugu_node node;
    umugu_sample *audio_source;
    size_t sample_capacity;
    char filename[UMUGU_PATH_LEN];
    void *file_handle;
} um__wav_player;

umugu_node_fn um__wavplayer_getfn(umugu_code fn);

/* MIXER */
typedef struct {
    umugu_node node;
    int32_t input_count;
    int32_t padding;
} um__mixer;

umugu_node_fn um__mixer_getfn(umugu_code fn);

/* AMPLITUDE */
typedef struct {
    umugu_node node;
    float multiplier;
    int32_t padding;
} um__amplitude;

umugu_node_fn um__amplitude_getfn(umugu_code fn);

/* LIMITER */
typedef struct {
    umugu_node node;
    float min;
    float max;
} um__limiter;

umugu_node_fn um__limiter_getfn(umugu_code fn);

/* INTERNAL HELPERS */
static inline void um__unused(void *unused_var) { (void)unused_var; }

#endif /* __UMUGU_INTERNAL_H__ */
