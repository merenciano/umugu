#ifndef __UMUGU_BUILTIN_NODES_H__
#define __UMUGU_BUILTIN_NODES_H__

#include "umugu/umugu.h"

#include <stddef.h>
#include <stdint.h>

#define UM__ARRAY_COUNT(ARR) ((int32_t)(sizeof(ARR) / sizeof(*(ARR))))

/* Oscilloscope - umugu/src/nodes/oscilloscope.c */
typedef struct {
    umugu_node node;
    int32_t phase;
    int32_t freq;
    int32_t waveform;
} um__oscope;

extern const umugu_var_info um__oscope_vars[];
extern const int32_t um__oscope_size;
extern const int32_t um__oscope_var_count;
extern umugu_node_fn um__oscope_getfn(umugu_fn fn);

/* Wav file player - umugu/src/nodes/wav_player.c */
typedef struct {
    umugu_node node;
    umugu_generic_signal wav;
    char filename[UMUGU_PATH_LEN];
    void *file_handle;
} um__wavplayer;

extern const umugu_var_info um__wavplayer_vars[];
extern const int32_t um__wavplayer_size;
extern const int32_t um__wavplayer_var_count;
extern umugu_node_fn um__wavplayer_getfn(umugu_fn fn);

/* Amplitude - umugu/src/nodes/amplitude.c */
typedef struct {
    umugu_node node;
    float multiplier;
    int32_t padding;
} um__amplitude;

extern const umugu_var_info um__amplitude_vars[];
extern const int32_t um__amplitude_size;
extern const int32_t um__amplitude_var_count;
extern umugu_node_fn um__amplitude_getfn(umugu_fn fn);

/* Limiter - umugu/src/nodes/limiter.c */
typedef struct {
    umugu_node node;
    float min;
    float max;
} um__limiter;

extern const umugu_var_info um__limiter_vars[];
extern const int32_t um__limiter_size;
extern const int32_t um__limiter_var_count;
extern umugu_node_fn um__limiter_getfn(umugu_fn fn);

/* Mixer - umugu/src/nodes/mixer.c */
#define UMUGU_MIXER_MAX_INPUTS 8
typedef struct {
    umugu_node node;
    int32_t input_count;
    int16_t extra_pipe_in_node_idx[UMUGU_MIXER_MAX_INPUTS];
    int8_t extra_pipe_in_channel[UMUGU_MIXER_MAX_INPUTS];
} um__mixer;

extern const umugu_var_info um__mixer_vars[];
extern const int32_t um__mixer_size;
extern const int32_t um__mixer_var_count;
extern umugu_node_fn um__mixer_getfn(umugu_fn fn);

/* Piano - umugu/src/nodes/piano.c */
typedef struct {
    umugu_node node;
    int32_t waveform;
    float phase[UMUGU_NOTE_COUNT];
} um__piano;

extern const umugu_var_info um__piano_vars[];
extern const int32_t um__piano_size;
extern const int32_t um__piano_var_count;
extern umugu_node_fn um__piano_getfn(umugu_fn fn);

/* Output - umugu/src/nodes/output.c */
typedef struct {
    umugu_node node;
} um__output;

extern const umugu_var_info um__output_vars[];
extern const int32_t um__output_size;
extern const int32_t um__output_var_count;
extern umugu_node_fn um__output_getfn(umugu_fn fn);

#endif /* __UMUGU_BUILTIN_NODES_H__ */

#ifdef UMUGU_BUILTIN_NODES_DEFINITIONS
const umugu_var_info um__oscope_vars[] = {
    {.name = {.str = "Frequency"},
     .offset_bytes = offsetof(um__oscope, freq),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .misc.rangef.min = 1,
     .misc.rangef.max = 8372},
    {.name = {.str = "Waveform"},
     .offset_bytes = offsetof(um__oscope, waveform),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .misc.rangei.min = 0,
     .misc.rangei.max = UMUGU_WAVEFORM_COUNT}};
const int32_t um__oscope_size = (int32_t)sizeof(um__oscope);
const int32_t um__oscope_var_count = UM__ARRAY_COUNT(um__oscope_vars);

const umugu_var_info um__wavplayer_vars[] = {
    {.name = {.str = "Channels"},
     .offset_bytes = offsetof(um__wavplayer, wav) +
                     offsetof(umugu_generic_signal, channels),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .flags = UMUGU_VAR_RDONLY},
    {.name = {.str = "Sample rate"},
     .offset_bytes =
         offsetof(um__wavplayer, wav) + offsetof(umugu_generic_signal, rate),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .flags = UMUGU_VAR_RDONLY},
    {.name = {.str = "Sample data type"},
     .offset_bytes =
         offsetof(um__wavplayer, wav) + offsetof(umugu_generic_signal, type),
     .type = UMUGU_TYPE_INT16,
     .count = 1,
     .flags = UMUGU_VAR_RDONLY},
    {.name = {.str = "Filename"},
     .offset_bytes = offsetof(um__wavplayer, filename),
     .type = UMUGU_TYPE_TEXT,
     .count = UMUGU_PATH_LEN}};
const int32_t um__wavplayer_size = (int32_t)sizeof(um__wavplayer);
const int32_t um__wavplayer_var_count = UM__ARRAY_COUNT(um__wavplayer_vars);

const umugu_var_info um__amplitude_vars[] = {
    {.name = {.str = "Multiplier"},
     .offset_bytes = offsetof(um__amplitude, multiplier),
     .type = UMUGU_TYPE_FLOAT,
     .count = 1,
     .misc.rangef.min = 0.0f,
     .misc.rangef.max = 5.0f}};
const int32_t um__amplitude_size = (int32_t)sizeof(um__amplitude);
const int32_t um__amplitude_var_count = UM__ARRAY_COUNT(um__amplitude_vars);

const umugu_var_info um__limiter_vars[] = {
    {.name = {.str = "Min"},
     .offset_bytes = offsetof(um__limiter, min),
     .type = UMUGU_TYPE_FLOAT,
     .count = 1,
     .misc.rangef.min = -5.0f,
     .misc.rangef.max = 5.0f},
    {.name = {.str = "Max"},
     .offset_bytes = offsetof(um__limiter, max),
     .type = UMUGU_TYPE_FLOAT,
     .count = 1,
     .misc.rangef.min = -5.0f,
     .misc.rangef.max = 5.0f}};
const int32_t um__limiter_size = (int32_t)sizeof(um__limiter);
const int32_t um__limiter_var_count = UM__ARRAY_COUNT(um__limiter_vars);

const umugu_var_info um__mixer_vars[] = {
    {.name = {.str = "InputCount"},
     .offset_bytes = offsetof(um__mixer, input_count),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .misc.rangei.min = 0,
     .misc.rangei.max = UMUGU_MIXER_MAX_INPUTS},
    {.name = {.str = "ExtraInputPipeNodeIdx"},
     .offset_bytes = offsetof(um__mixer, extra_pipe_in_node_idx),
     .type = UMUGU_TYPE_INT16,
     .count = UMUGU_MIXER_MAX_INPUTS,
     .misc.rangei.min = 0,
     .misc.rangei.max = 9999},
    {.name = {.str = "ExtraInputPipeChannel"},
     .offset_bytes = offsetof(um__mixer, extra_pipe_in_channel),
     .type = UMUGU_TYPE_INT8,
     .count = UMUGU_MIXER_MAX_INPUTS,
     .misc.rangei.min = 0,
     .misc.rangei.max = UMUGU_MIXER_MAX_INPUTS}};
const int32_t um__mixer_size = (int32_t)sizeof(um__mixer);
const int32_t um__mixer_var_count = UM__ARRAY_COUNT(um__mixer_vars);

const umugu_var_info um__piano_vars[] = {
    {.name = {.str = "Waveform"},
     .offset_bytes = offsetof(um__piano, waveform),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .misc.rangei.min = 0,
     .misc.rangei.max = UMUGU_WAVEFORM_COUNT}};
const int32_t um__piano_size = (int32_t)sizeof(um__piano);
const int32_t um__piano_var_count = UM__ARRAY_COUNT(um__piano_vars);

const umugu_var_info um__output_vars[] = {
    {.name = {.str = "Input node"},
     .offset_bytes = offsetof(umugu_node, in_pipe_node),
     .type = UMUGU_TYPE_INT16,
     .count = 0,
     .flags = UMUGU_VAR_RDONLY,
     .misc.rangei.min = -1,
     .misc.rangei.max = -1}};
const int32_t um__output_size = (int32_t)sizeof(um__output);
const int32_t um__output_var_count = UM__ARRAY_COUNT(um__output_vars);

#endif /* UMUGU_BUILTIN_NODES_DEFINITIONS */
