#ifndef __UMUGU_INTERNAL_H__
#define __UMUGU_INTERNAL_H__

#include <umugu/umugu.h>

#include <string.h> /* memset, memcmp, strncpy */

// #define UMUGU_DEBUG
// #define UMUGU_TRACE
// #define UMUGU_VERBOSE

#define UM_UNUSED(VAR) (void)(VAR)

#ifndef UMUGU_ASSERT
#include <assert.h>
#define UMUGU_ASSERT assert
#endif

// UMUGU_TRAP
#ifndef UMUGU_TRAP
#undef UM_HAS_BUILTIN_TRAP
#ifdef __has_builtin
#if __has_builtin(__builtin_trap)
#define UM_HAS_BUILTIN_TRAP
#endif
#else
#ifdef __GNUC__
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
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
#endif
// END UMUGU_TRAP

#ifdef __cplusplus
extern "C" {
#endif

typedef int umugu_fn; /* enum umugu_fn_ */

/**
 * Initializes the current context's pipeline using the given names and default
 * values.
 * @param ctx Pointer to the context instance where generate the pipeline.
 * @param names Array of the desired node types.
 * @param count Number of names in the node_names array.
 */
UMUGU_API int um_pipeline_generate(umugu_ctx *ctx, const umugu_name *names, int count);

/* Search the file lib<name>.so in the rpath and load it if found.
 * Return the index of the context's node infos array where it has been copied.
 * If the dynamic object can not be found, return UMUGU_ERR_PLUG. */
UMUGU_API int um_node_plug(umugu_ctx *ctx, const umugu_name *name);
UMUGU_API void um_node_unplug(umugu_node_type_info *info);

/* Node virtual dispatching.
 * @param fn Function identifier, valid definitions are prefixed with UMUGU_FN_.
 * Return UMUGU_SUCCESS if the call is performed and UMUGU_ERR otherwise. */
UMUGU_API int
umugu_node_dispatch(umugu_ctx *ctx, umugu_node *node, umugu_fn fn, umugu_fn_flags flags);

/* Search the node name in the built-in nodes and add the node info to
 * the context. If there is no built-in node with that name, looks for
 * lib<node_name>.so in the rpath and plug it if found.
 * Return a pointer to the loaded node info in the context or NULL if not found.
 */
UMUGU_API const umugu_node_type_info *umugu_node_info_load(umugu_ctx *ctx, const umugu_name *name);

/**
 * @brief Persistent allocation.
 * The allocated buffer can not be released and will be valid until the context is unloaded.
 * The more persistent memory allocated the less available memory for temporal allocs.
 * This alloc is intended only for initialization time.
 * @param bytes Alloc size in bytes.
 * @return Allocated memory. It never returns NULL.
 */
UMUGU_API void *um_alloc_persistent(umugu_ctx *ctx, size_t bytes);

/**
 * @brief Temporal allocation.
 * The allocated memory will be automatically reassigned at some point after the current iteration
 * finishes, so do not read from it afterwards.
 * @note This allocator manages the remaining space after the permanent allocs as a ring buffer.
 * @param bytes Alloc size in bytes.
 * @return Allocated memory. It never returns NULL.
 */
UMUGU_API void *um_talloc(umugu_ctx *ctx, size_t bytes);

/**
 * Allocates a large enough sample buffer in the signal for the current iteration frame count.
 * @param samples Caller's owned sample buffer.
 * @return Convenience ptr to the allocated memory inside @ref samples. Can be discarded.
 */
UMUGU_API float *um_alloc_samples(umugu_ctx *ctx, umugu_samples *samples);

/**
 * @copydoc um_alloc_signal
 * @return The allocated memory, can be discarded since the signal is pointing to it.
 */
UMUGU_API void *um_alloc_signal(umugu_ctx *ctx, umugu_signal *signal);

/* Node functions. */
enum umugu_fn_ {
    UMUGU_FN_INIT,
    UMUGU_FN_PROCESS,
    UMUGU_FN_RELEASE,
};

#ifdef __cplusplus
}
#endif

/* ## GENERAL UTILS ## */

/* nanosecond chrono */
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

/* name API (fixed string of 32 bytes) */
static inline bool
um_name_empty(const umugu_name *name)
{
    return !*name->str;
}

static inline bool
um_name_equals(const umugu_name *a, const umugu_name *b)
{
    return !memcmp(a, b, UMUGU_NAME_LEN);
}

static inline void
um_name_clear(umugu_name *self)
{
    memset(self->str, 0, UMUGU_NAME_LEN);
}

static inline const char *
um_name_strcpy(umugu_name *dst, const char *src)
{
    um_name_clear(dst);
    return strncpy(dst->str, src, UMUGU_NAME_LEN - 1);
}

uint64_t um_name_hash(const umugu_name *name);

static inline int
um_type_sizeof(int type)
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
        UMUGU_ASSERT(0);
        return 0;
    }
}

/*  ##  PIPELINE NODE HELPERS  ##  */
static inline const umugu_node *
um_node_get_input(umugu_ctx *ctx, const umugu_node *node)
{
    int idx = node->prev_node;
    if (idx < 0) {
        return NULL;
    }

    return ctx->pipeline.nodes[idx];
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
um_signal_stride(const umugu_signal *signal)
{
    UMUGU_ASSERT(signal);
    int size = um_type_sizeof(signal->format);
    return signal->interleaved_channels ? size * signal->samples.channel_count : size;
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
um_signal_offset(const umugu_signal *signal)
{
    UMUGU_ASSERT(signal);
    int size = um_type_sizeof(signal->format);
    return signal->interleaved_channels ? size : size * signal->samples.frame_count;
}

static inline void *
um_signal_sample(const umugu_signal *signal, int frame, int channel)
{
    int stride = um_signal_stride(signal) * frame;
    int offset = um_signal_offset(signal) * channel;
    return (char *)signal->samples.samples + offset + stride;
}

static inline float *
um_signal_get_channel(const umugu_samples *signal, int channel)
{
    UMUGU_ASSERT(signal && signal->samples && signal->frame_count > 0 && signal->channel_count > 0);
    return signal->samples + signal->frame_count * channel;
}

static inline float
um_signal_samplef(const umugu_signal *sig, int frame, int ch)
{
    void *sample = um_signal_sample(sig, frame, ch);
    switch (sig->format) {
    case UMUGU_TYPE_UINT8:
        return ((float)(*(uint8_t *)sample) / 255.0f) * 2.0f - 1.0f;
    case UMUGU_TYPE_INT16:
        return (float)(*(int16_t *)sample) / 32768.0f;
    case UMUGU_TYPE_FLOAT:
        return *(float *)sample;
    default:
        UMUGU_ASSERT(0);
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

void um_oscillator_sine(um_oscillator *self, umugu_samples *sig, int sample_rate);
void um_oscillator_sawsin(um_oscillator *self, umugu_samples *sig);
void um_oscillator_saw(um_oscillator *self, umugu_samples *sig, int sample_rate);
void um_oscillator_triangle(um_oscillator *self, umugu_samples *sig, int sample_rate);
void um_oscillator_square(um_oscillator *self, umugu_samples *sig, int sample_rate);
void um_noisegen_white(um_noisegen *self, umugu_samples *sig);

/* ## MATH ## */
static inline float
um_minf(float a, float b)
{
    return a > b ? b : a;
}

static inline float
um_maxf(float a, float b)
{
    return b > a ? b : a;
}

static inline int
um_mini(int a, int b)
{
    return a > b ? b : a;
}

static inline int
um_maxi(int a, int b)
{
    return b > a ? b : a;
}

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
    uint16_t waveform;
    uint16_t padding[3];
} um_oscil;

typedef struct {
    umugu_node node;
    umugu_signal wav;
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
} um_output;

umugu_node_func um_oscil_getfn(umugu_fn fn);
umugu_node_func um_wavplayer_getfn(umugu_fn fn);
umugu_node_func um_amplitude_getfn(umugu_fn fn);
umugu_node_func um_limiter_getfn(umugu_fn fn);
umugu_node_func um_mixer_getfn(umugu_fn fn);
umugu_node_func um_output_getfn(umugu_fn fn);

#endif /* __UMUGU_INTERNAL_H__ */
