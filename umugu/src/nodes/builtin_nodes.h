#include "umugu/umugu.h"

#include <stddef.h>
#include <stdint.h>

/* Oscilloscope - umugu/src/nodes/oscilloscope.c */
extern const int32_t um__oscope_size;
extern const int32_t um__oscope_var_count;
extern const umugu_var_info um__oscope_vars[];
extern umugu_node_fn um__oscope_getfn(umugu_code fn);

/* Wav file player - umugu/src/nodes/wav_player.c */
extern const int32_t um__wavplayer_size;
extern const int32_t um__wavplayer_var_count;
extern const umugu_var_info um__wavplayer_vars[];
extern umugu_node_fn um__wavplayer_getfn(umugu_code fn);

/* Amplitude - umugu/src/nodes/amplitude.c */
extern const int32_t um__amplitude_size;
extern const int32_t um__amplitude_var_count;
extern const umugu_var_info um__amplitude_vars[];
extern umugu_node_fn um__amplitude_getfn(umugu_code fn);

/* Limiter - umugu/src/nodes/limiter.c */
extern const int32_t um__limiter_size;
extern const int32_t um__limiter_var_count;
extern const umugu_var_info um__limiter_vars[];
extern umugu_node_fn um__limiter_getfn(umugu_code fn);

/* Mixer - umugu/src/nodes/mixer.c */
extern const int32_t um__mixer_size;
extern const int32_t um__mixer_var_count;
extern const umugu_var_info um__mixer_vars[];
extern umugu_node_fn um__mixer_getfn(umugu_code fn);

enum { UM__BUILTIN_NODES_COUNT = 5 };
extern umugu_node_info g_builtin_nodes_info[UM__BUILTIN_NODES_COUNT];

/* Waveform helpers - umugu/src/nodes/waveform.c */
enum {
    UM__WAVEFORM_SINE = 0,
    UM__WAVEFORM_SAW,
    UM__WAVEFORM_SQUARE,
    UM__WAVEFORM_TRIANGLE,
    UM__WAVEFORM_WHITE_NOISE,
    UM__WAVEFORM_COUNT
};

/* Look-up table with waveform signals. TODO: mmap binary file instead of
 * compiling .c */
extern const float um__waveform_lut[UM__WAVEFORM_COUNT][UMUGU_SAMPLE_RATE];

// TODO: Generate a binary file for mmaping instead of a .c
// TODO: Merge gen and fill (instead of filling, write to the file).
// Exports the wave look-up table to a c file.
// The file will be formatted and the variable named like the existing
// osciloscope_lut.c. Remember to call umugu__fill_wave_lut before exporting, if
// you don't the generated file will be exactly the same as osciloscope_lut.c.
void um__gen_wave_file(const char *filename);
// Recalculate the wave look-up table values.
void um__fill_wave_lut(void);
