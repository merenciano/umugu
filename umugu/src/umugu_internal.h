#ifndef __UMUGU_INTERNAL_H__
#define __UMUGU_INTERNAL_H__

#include <umugu/umugu.h>

#include <assert.h>

#undef UM_HAS_BUILTIN_TRAP
#ifdef __has_builtin
#if __has_builtin(__builtin_trap)
#define UM_HAS_BUILTIN_TRAP
#endif
#else
#ifdef __GNUC__
#define GCC_VERSION                                                            \
    (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#if GCC_VERSION > 40203
#define UM_HAS_BUILTIN_TRAP
#endif
#endif
#endif

#ifdef UM_HAS_BUILTIN_TRAP
#define UMUGU_TRAP() __builtin_trap()
#else
#include <signal.h>
#define UMUGU_TRAP() raise(SIGTRAP);
#endif

/* ## GENERAL UTILS ## */

typedef int64_t um_nanosec;
um_nanosec um_time_now(void);
static inline float
um_time_sec(um_nanosec ns)
{
    return ns / 1000000000.0f;
}
static inline int64_t
um_time_ms(um_nanosec ns)
{
    return ns / 1000000;
}
static inline int64_t
um_time_micro(um_nanosec ns)
{
    return ns / 1000;
}
static inline um_nanosec
um_time_elapsed(um_nanosec ns)
{
    return um_time_now() - ns;
}

static inline int
um_sizeof(int type)
{
    switch (type) {
    case UMUGU_TYPE_INT8:
    case UMUGU_TYPE_UINT8:
    case UMUGU_TYPE_TEXT:
    case UMUGU_TYPE_BOOL:
        return 1;
    case UMUGU_TYPE_INT16:
    case UMUGU_TYPE_UINT16:
        return 2;
    case UMUGU_TYPE_INT32:
    case UMUGU_TYPE_UINT32:
    case UMUGU_TYPE_FLOAT:
        return 4;
    case UMUGU_TYPE_INT64:
    case UMUGU_TYPE_UINT64:
    case UMUGU_TYPE_DOUBLE:
        return 8;
    default:
        assert(0);
        return 0;
    }
}

/*  ##  PIPELINE NODE HELPERS  ##  */
static inline void
um_node_check_iteration(const umugu_node *node)
{
    assert((node->iteration < umugu_get_context()->pipeline_iteration) &&
           "The node iteration should be less than the pipeline_iteration "
           "before being processed, and equal when its signal has been "
           "generated. Are you trying to process the same node twice in the "
           "same pipeline iteration?.");
}

static inline const umugu_node *
um_node_get_input(const umugu_node *node)
{
    int idx = node->prev_node;
    if (idx < 0) {
        return NULL;
    }

    umugu_node *in = umugu_get_context()->pipeline.nodes[idx];
    assert((in && (in->iteration > node->iteration)) &&
           "The input node should be at least one iteration ahead.");
    return in;
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
static inline int
um_generic_signal_stride(const umugu_generic_signal *signal)
{
    assert(signal);
    int size = um_sizeof(signal->type);
    uint32_t interleaved = signal->flags & UMUGU_SIGNAL_INTERLEAVED;
    return signal->flags & interleaved ? size * signal->channels : size;
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
static inline int
um_generic_signal_offset(const umugu_generic_signal *signal)
{
    assert(signal);
    int size = um_sizeof(signal->type);
    uint32_t interleaved = signal->flags & UMUGU_SIGNAL_INTERLEAVED;
    return interleaved ? size : size * signal->count;
}

static inline void *
um_generic_signal_sample(const umugu_generic_signal *signal, int frame,
                         int channel)
{
    int stride = um_generic_signal_stride(signal) * frame;
    int offset = um_generic_signal_offset(signal) * channel;
    return (char *)signal->sample_data + offset + stride;
}

static inline umugu_sample *
um_signal_get_channel(const umugu_signal *signal, int channel)
{
    assert(signal && signal->samples && signal->count > 0 &&
           signal->channels > 0);
    return signal->samples + signal->count * channel;
}

static inline float
um_generic_signal_samplef(const umugu_generic_signal *sig, int frame, int ch)
{
    void *sample = um_generic_signal_sample(sig, frame, ch);
    switch (sig->type) {
    case UMUGU_TYPE_UINT8:
        return ((float)(*(uint8_t *)sample) / 255.0f) * 2.0f - 1.0f;
    case UMUGU_TYPE_INT16:
        return (float)(*(int16_t *)sample) / 32768.0f;
    case UMUGU_TYPE_FLOAT:
        return *(float *)sample;
    default:
        assert(0);
        return 0.0f;
    }
}

/* ## WAVEFORM GENERATION ## */

typedef struct um_oscillator {
    float freq;  /* hz */
    float phase; /* rads */
} um_oscillator;

typedef struct um_noisegen {
    int32_t x1; /* general purpose state vars */
    int32_t x2;
} um_noisegen;

void um_oscillator_sine(um_oscillator *self, umugu_signal *sig);
void um_oscillator_sawsin(um_oscillator *self, umugu_signal *sig);
void um_oscillator_saw(um_oscillator *self, umugu_signal *sig);
void um_oscillator_triangle(um_oscillator *self, umugu_signal *sig);
void um_oscillator_square(um_oscillator *self, umugu_signal *sig);
void um_noisegen_white(um_noisegen *self, umugu_signal *sig);

/* ## MATH ## */

static inline float
um_lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

typedef struct {
    float real;
    float imag;
} um_complex;

void um_fft(um_complex *v, int n, um_complex *tmp);
void um_ifft(um_complex *v, int n, um_complex *tmp);

/* ## NOTES ## */

float um_note_freq(int note_index);

/* ## BUILT-IN PIPELINE NODES ## */

typedef struct {
    umugu_node node;
    um_oscillator osc;
    um_noisegen noise;
    uint16_t waveform; /* UMUGU_SIGNAL_ */
    uint16_t padding[3];
} um_oscil;

typedef struct {
    umugu_node node;
    umugu_generic_signal wav;
    char filename[UMUGU_PATH_LEN];
    void *file_handle;
} um_wavplayer;

typedef struct {
    umugu_node node;
    float multiplier;
    int32_t padding;
} um_amplitude;

typedef struct {
    umugu_node node;
    float min;
    float max;
} um_limiter;

typedef struct {
    umugu_node node;
    int32_t input_count;
    int16_t extra_pipe_in_node_idx[UMUGU_MIXER_MAX_INPUTS];
    int8_t extra_pipe_in_channel[UMUGU_MIXER_MAX_INPUTS];
} um_mixer;

typedef struct {
    umugu_node node;
    int32_t waveform;
    um_oscillator osc[UMUGU_NOTE_COUNT];
} um_piano;

typedef struct {
    umugu_node node;
} um_output;

umugu_node_fn um_oscil_getfn(umugu_fn fn);
umugu_node_fn um_wavplayer_getfn(umugu_fn fn);
umugu_node_fn um_amplitude_getfn(umugu_fn fn);
umugu_node_fn um_limiter_getfn(umugu_fn fn);
umugu_node_fn um_mixer_getfn(umugu_fn fn);
umugu_node_fn um_piano_getfn(umugu_fn fn);
umugu_node_fn um_output_getfn(umugu_fn fn);

#endif /* __UMUGU_INTERNAL_H__ */
