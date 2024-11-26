#include "umugu.h"

#include "umugu_internal.h"

#include <dlfcn.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define UMUGU_CONFIG_MAP_SIZE 32
#define UMUGU_CONFIG_VALUE_LEN 32 // Average value length for string storage buffer size.

#define UM_ARRAY_SIZE(ARR) (sizeof(ARR) / sizeof(ARR[0]))

#define UM_PTR(PTR, OFFSET_BYTES) ((void *)((char *)((void *)(PTR)) + (OFFSET_BYTES)))

#define UM_DIST_BYTES(HEAD_PTR, TAIL_PTR)                                                          \
    (((char *)((void *)(TAIL_PTR))) - ((char *)((void *)(HEAD_PTR))))

#define UM_PI 3.14159265358979323846
#define UM_TWOPI 2.0 * UM_PI

#define UM_AS_NAME(LITERAL) ((umugu_name){.str = LITERAL})

// Defaults
static const umugu_type UM_DEFAULT_SAMPLE_FORMAT = UMUGU_TYPE_FLOAT;
static const int UM_DEFAULT_SAMPLE_RATE = 48000;
static const int UM_DEFAULT_CHANNELS = 2;
static const bool UM_DEFAULT_INTERLEAVED_CHANNELS = true;
static const bool UM_DEFAULT_AUDIO_IN = false;
static const bool UM_DEFAULT_AUDIO_OUT = true;

/*
 *  PRIVATE FUNCTIONS
 */
typedef struct um_confmap um_confmap;
static const umugu_node_type_info *um_node_info_builtin_find(const umugu_name *name);
static int um_load_config(umugu_ctx *ctx, const char *filename);
static void um_load_defaults(umugu_ctx *ctx);
static int um_confmap_insert(um_confmap *cm, const umugu_name *key, const char *value, size_t len);
static const char *um_confmap_get(const um_confmap *cm, const umugu_name *key);

const umugu_node_type_info *
umugu_node_info_load(umugu_ctx *ctx, const umugu_name *name)
{
    UMUGU_ASSERT(ctx && name);
    /* Check if the node info is already loaded. */
    for (int i = 0; i < ctx->nodes_info_next; ++i) {
        if (um_name_equals(&ctx->nodes_info[i].name, name)) {
            return ctx->nodes_info + i;
        }
    }

    /* Look for it in the built-in nodes. */
    const umugu_node_type_info *bi_info = um_node_info_builtin_find(name);
    if (bi_info) {
        /* Add the node info to the current context. */
        UMUGU_ASSERT(ctx->nodes_info_next < UMUGU_DEFAULT_NODE_INFO_CAPACITY);
        umugu_node_type_info *info = &ctx->nodes_info[ctx->nodes_info_next++];
        memcpy(info, bi_info, sizeof(umugu_node_type_info));
#ifdef UMUGU_VERBOSE
        ctx->io.log("Built-in node %s loaded.\n", name->str);
#endif
        return info;
    }

    /* Check if there's a plug with that name and load it. */
    int ret = um_node_plug(ctx, name);
    if (ret >= 0) {
        return ctx->nodes_info + ret;
    }

    /* Node info not found. */
    UMUGU_ASSERT(ret == UMUGU_ERR_PLUG);
    ctx->io.log("Node type %s not found.\n", name->str);
    return NULL;
}

umugu_ctx *
umugu_load(const umugu_config *cfg)
{
    um_nanosec init_time = um_time_now();
    UMUGU_ASSERT(cfg);
    UMUGU_ASSERT(cfg->arena && cfg->arena_size > sizeof(umugu_ctx));
    UMUGU_ASSERT(cfg->arena);
    memset(cfg->arena, 0, cfg->arena_size);

    // Persistent alloc for context
    umugu_ctx *ctx = cfg->arena;
    ctx->arena_head = cfg->arena;
    ctx->arena_tail = ctx->arena_head + sizeof(umugu_ctx);
    ctx->arena_capacity = cfg->arena_size;
    ctx->arena_pers_end = ctx->arena_tail;

    ctx->state = UMUGU_STATE_LOADING;

    ctx->io.log = cfg->log_fn;
    ctx->io.fatal = cfg->fatal_err_fn;
    ctx->io.file_read = cfg->load_file_fn;

    int err = um_load_config(ctx, cfg->config_file);
    if (err != UMUGU_SUCCESS) {
        um_load_defaults(ctx);
    }

    if (!ctx->pipeline.node_count) {
        um_pipeline_generate(ctx, cfg->fallback_ppln, cfg->fallback_ppln_node_count);
    }

    ctx->init_time_ns = um_time_elapsed(init_time);
    ctx->state = UMUGU_STATE_IDLE;

#ifdef UMUGU_VERBOSE
    ctx->io.log("Umugu initialized in %ldns\n", ctx->init_time_ns);
#endif

    return ctx;
}

void
umugu_unload(umugu_ctx *ctx)
{
    if (!ctx) {
        return;
    }

    ctx->state = UMUGU_STATE_UNLOADING;
    for (int i = 0; i < ctx->pipeline.node_count; ++i) {
        umugu_node_dispatch(ctx, ctx->pipeline.nodes[i], UMUGU_FN_RELEASE, UMUGU_NOFLAG);
    }

    for (int i = 0; i < ctx->nodes_info_next; ++i) {
        if (ctx->nodes_info[i].plug_handle) {
            um_node_unplug(&ctx->nodes_info[i]);
        }
    }

    ctx->state = UMUGU_STATE_INVALID;
}

UMUGU_API int
umugu_process(umugu_ctx *ctx, size_t frames, umugu_signal *in, umugu_signal *out)
{
    UMUGU_ASSERT(ctx);
    ctx->pipeline.sig.samples.frame_count = frames;
    ctx->state = UMUGU_STATE_PROCESSING;
    if (!ctx->ppln_iterations) {
        for (int i = 0; i < ctx->pipeline.node_count; ++i) {
            umugu_node *node = ctx->pipeline.nodes[i];
            int err = umugu_node_dispatch(ctx, node, UMUGU_FN_INIT, UMUGU_NOFLAG);
            if (err < UMUGU_SUCCESS) {
                ctx->io.log(
                    "Error (%d) processing node:\n"
                    "\tIndex: %d.\n\tName: %s\n",
                    err, i, ctx->nodes_info[node->info_idx].name);
            }
        }
    }

    ctx->ppln_iterations++;
    ctx->ppln_it_allocated = 0;

    if (in) {
        ctx->io.in_audio = *in;
    }
    if (out) {
        ctx->io.out_audio = *out;
    }

    for (int i = 0; i < ctx->pipeline.node_count; ++i) {
        umugu_node *node = ctx->pipeline.nodes[i];
        int err = umugu_node_dispatch(ctx, node, UMUGU_FN_PROCESS, UMUGU_NOFLAG);
        if (err < UMUGU_SUCCESS) {
            UMUGU_TRAP();
            ctx->io.log(
                "Error (%d) processing node:\n"
                "\tIndex: %d.\n\tName: %s\n",
                err, i, ctx->nodes_info[node->info_idx].name);
        }
    }

    ctx->state = UMUGU_STATE_IDLE;
    return UMUGU_SUCCESS;
}

void *
um_alloc_persistent(umugu_ctx *ctx, size_t bytes)
{
    UMUGU_ASSERT(ctx);
    if (ctx->state > UMUGU_STATE_IDLE) {
        ctx->io.log(
            "At this point of the execution, every required"
            " permanent allocation should have been done yet.\n");

        UMUGU_ASSERT(
            (ctx->state != UMUGU_STATE_PROCESSING) &&
            "Permanent allocations are not permitted during pipeline "
            "processing, "
            "since the temporary allocs are heavily used and the new permanent "
            "allocations expand the buffer stealing space from that buffer.");
    }

    uint8_t *ret = ctx->arena_pers_end;
    register const uint8_t *const arena_end = ctx->arena_head + ctx->arena_capacity;
    if ((ret + bytes) > arena_end) {
        UMUGU_ASSERT(0 && "Fatal error: Persistent alloc failed. No space left in the arena.");
        ctx->io.fatal(
            UMUGU_ERR_MEM, "Fatal error: Persistent alloc failed. No space left in the arena.\n",
            __FILE__, __LINE__);
        UMUGU_TRAP();
    }
    ctx->arena_pers_end += bytes;
    ctx->arena_tail = ctx->arena_pers_end;
    UMUGU_ASSERT(ctx->arena_pers_end < arena_end);
    return ret;
}

void *
um_talloc(umugu_ctx *ctx, size_t bytes)
{
    UMUGU_ASSERT(ctx);
    if (ctx->state < UMUGU_STATE_IDLE) {
        ctx->io.log(
            "Careful with temporary allocations during"
            " initialization since the next persistent allocation will "
            "steal that region. It is OK for local use but are you sure"
            " you can not use the stack in this situation?\n");
    }

    UMUGU_ASSERT(ctx->arena_tail >= ctx->arena_pers_end);
    UMUGU_ASSERT(ctx->arena_tail <= (ctx->arena_head + ctx->arena_capacity));
    register const uint8_t *const arena_end = ctx->arena_head + ctx->arena_capacity;

    uint8_t *ret = ctx->arena_tail;
    if ((ret + bytes) > arena_end) {
        /* Arena limit reached, wrap back after the last persistent region. */
        ret = ctx->arena_pers_end;
        if ((ret + bytes) > arena_end) {
            UMUGU_ASSERT(0 && "Fatal error: Temporal alloc failed. No space left in the arena.");
            ctx->io.fatal(
                UMUGU_ERR_MEM, "Fatal error: Temporal alloc failed. No space left in the arena.\n",
                __FILE__, __LINE__);
            UMUGU_TRAP();
        }
    }

    ctx->arena_tail = ret + bytes;
    ctx->ppln_it_allocated += bytes;
    return ret;
}

float *
um_alloc_samples(umugu_ctx *ctx, umugu_samples *s)
{
    UMUGU_ASSERT(s);
    UMUGU_ASSERT(s->channel_count > 0 && s->channel_count < 32 && "Invalid number of channels.");
    s->frame_count = ctx->pipeline.sig.samples.frame_count;
    UMUGU_ASSERT(s->frame_count > 0);
    s->samples = um_talloc(ctx, s->frame_count * sizeof(s->samples[0]) * s->channel_count);
    return s->samples;
}

void *
um_alloc_signal(umugu_ctx *ctx, umugu_signal *s)
{
    UMUGU_ASSERT(s);
    s->samples.frame_count = ctx->pipeline.sig.samples.frame_count;
    UMUGU_ASSERT(s->samples.frame_count > 0);
    s->samples.samples = um_talloc(
        ctx, s->samples.frame_count * um_type_sizeof(s->format) * s->samples.channel_count);
    return s->samples.samples;
}

int
umugu_node_dispatch(umugu_ctx *ctx, umugu_node *node, umugu_fn fn, umugu_fn_flags flags)
{
    UMUGU_ASSERT(node->info_idx >= 0 && "Node info index can not be negative.");
    UMUGU_ASSERT(ctx->nodes_info_next > node->info_idx && "Node info not loaded.");
    umugu_node_func func = ctx->nodes_info[node->info_idx].getfn(fn);
    return func ? func(ctx, node, flags) : UMUGU_ERR_NULL;
}

int
um_node_plug(umugu_ctx *ctx, const umugu_name *name)
{
    char buf[1024] = {"lib"};
    strncpy(&buf[3], name->str, UMUGU_NAME_LEN);
    char *it = &buf[3];
    while (*it) {
    }
    it[0] = '.';
    it[1] = 's';
    it[2] = 'o';
    it[3] = '\0';
    // snprintf(buf, 1024, "lib%s.so", name->str);

    assert(ctx->nodes_info_next < UMUGU_DEFAULT_NODE_INFO_CAPACITY);
    void *hnd = dlopen(buf, RTLD_NOW);
    if (!hnd) {
        ctx->io.log("Can't load plug: dlopen(%s) failed.", buf);
        return UMUGU_ERR_PLUG;
    }

    umugu_node_type_info *info = &ctx->nodes_info[ctx->nodes_info_next++];
    info->name = *name;
    *(void **)&info->getfn = dlsym(hnd, "getfn");
    info->size_bytes = *(int32_t *)dlsym(hnd, "size");
    info->attribs = *(umugu_attrib_info **)dlsym(hnd, "attribs");
    info->attrib_count = *(int32_t *)dlsym(hnd, "attrib_count");
    info->plug_handle = hnd;

#ifdef UMUGU_VERBOSE
    ctx->io.log("Node plugged successfuly: %s\n", name->str);
#endif
    return ctx->nodes_info_next - 1;
}

void
um_node_unplug(umugu_node_type_info *info)
{
    if (info && info->plug_handle) {
        dlclose(info->plug_handle);
        memset(info, 0, sizeof(*info));
    }
}

/*  ***  UMUGU INTERNAL  ***  */

um_nanosec
um_time_now(void)
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_nsec + time.tv_sec * 1000000000;
}

static inline float
um_oscillator_phase(float phase, float freq, float sr)
{
    register float sum = phase + freq;
    return sum >= sr ? sum - sr : sum;
}

void
um_oscillator_sine(um_oscillator *self, umugu_samples *sig, int sample_rate)
{
    const float TWOPI = 2.0f * UM_PI;
    const float w = self->freq * TWOPI / sample_rate;
    const int count = sig->frame_count;
    sig->channel_count = 1;

    float b1 = 2.0f * cosf(w);
    sig->samples[0] = sinf(self->phase);
    sig->samples[1] = sinf(self->phase + w);
    for (int i = 2; i < count; ++i) {
        sig->samples[i] = b1 * sig->samples[i - 1] - sig->samples[i - 2];
    }

    self->phase = fmodf(self->phase + count * w, 2.0f * UM_PI);
}

void
um_oscillator_sawsin(um_oscillator *self, umugu_samples *sig)
{
    static const float TWOPI = 2.0f * UM_PI;
    const int count = sig->frame_count;
    sig->channel_count = 1;
    for (int i = 0; i < count; ++i) {
        float t = fmodf(self->phase / TWOPI, 1.0f);
        if (t > 0.5f) {
            sig->samples[i] = -sinf(self->phase);
        } else {
            sig->samples[i] = 2.0f * t - 1.0f;
        }
        self->phase += self->freq;
    }
}

void
um_oscillator_saw(um_oscillator *self, umugu_samples *sig, int sample_rate)
{
    const int count = sig->frame_count;
    const float sr = sample_rate;
    const float hsr = sr * 0.5f;
    sig->channel_count = 1;
    for (int i = 0; i < count; ++i) {
        float value = self->phase / hsr;
        sig->samples[i] = value > 1.0f ? value - 2.0f : value;
        self->phase = um_oscillator_phase(self->phase, self->freq, sr);
        self->phase += self->freq;
        if (self->phase >= sample_rate) {
            self->phase -= sample_rate;
        }
    }
}

void
um_oscillator_triangle(um_oscillator *self, umugu_samples *sig, int sample_rate)
{
    const float rate = sample_rate;
    const float rqsr = 1.0f / (rate / 4.0f);
    const int count = sig->frame_count;
    sig->channel_count = 1;
    static const float LUT[] = {0.0f, 1.0f, 0.0f, -1.0f, 0.0f};
    for (int i = 0; i < count; ++i) {
        float t = self->phase * rqsr;
        int ipart = (int)t;
        sig->samples[i] = um_lerp(LUT[ipart], LUT[ipart + 1], t - ipart);

        self->phase += self->freq;
        if (self->phase >= sample_rate) {
            self->phase -= sample_rate;
        }
    }
}

void
um_oscillator_square(um_oscillator *self, umugu_samples *sig, int sample_rate)
{
    const float w = self->freq * UM_TWOPI / sample_rate;
    const int count = sig->frame_count;
    sig->channel_count = 1;
    while (self->phase >= UM_TWOPI) {
        self->phase -= UM_TWOPI;
    }

    for (int i = 0; i < count; ++i) {
        sig->samples[i] = (self->phase / UM_TWOPI) < 0.5f ? 1.0f : -1.0f;
        float next_phase = self->phase + w;
        self->phase = next_phase < UM_TWOPI ? next_phase : next_phase - UM_TWOPI;
    }

    self->phase = fmodf(self->phase + count * w, 2.0f * UM_PI);
}

void
um_noisegen_white(um_noisegen *gen, umugu_samples *sig)
{
    assert(sig && (sig->frame_count > 0) && sig->channel_count == 1);
    assert(sig->samples);
    assert(gen);

    if (!gen->x1) {
        gen->x1 = 0x67452301;
    }

    if (!gen->x2) {
        gen->x2 = 0xefcdab89;
    }

    const int count = sig->frame_count;
    for (int i = 0; i < count; ++i) {
        gen->x1 ^= gen->x2;
        sig->samples[i] = gen->x2 * (2.0f / (float)0xffffffff);
        gen->x2 += gen->x1;
    }
}

float
um_note_freq(int note_index)
{
    const float um_note_freq[] = {
        261.63f, 277.18f, 293.66f, 311.13f, 329.63f, 349.23f,
        369.99f, 392.0f,  415.3f,  440.0f,  466.16f, 493.88f};
    const float um_note_oct_mul[] = {
        0.03125f, 0.0625f, 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f};
    int oct = note_index / 12;
    int note = note_index % 12;
    return um_note_freq[note] * um_note_oct_mul[oct];
}

/*
    fft(v,N):
    Factored discrete Fourier Transform.
    From https://www.math.wustl.edu/~victor/mfmm/fourier/fft.c
    [0] If N==1 then return.
    [1] For k = 0 to N/2-1, let ve[k] = v[2*k]
    [2] Compute fft(ve, N/2);
    [3] For k = 0 to N/2-1, let vo[k] = v[2*k+1]
    [4] Compute fft(vo, N/2);
    [5] For m = 0 to N/2-1, do [6] through [9]
    [6]   Let w.re = cos(2*M_PI*m/N)
    [7]   Let w.im = -sin(2*M_PI*m/N)
    [8]   Let v[m] = ve[m] + w*vo[m]
    [9]   Let v[m+N/2] = ve[m] - w*vo[m]
*/
void
um_fft(um_complex *v, int n, um_complex *tmp)
{
    if (n > 1) { /* otherwise, do nothing and return */
        int k, m;
        um_complex z, w, *vo, *ve;
        ve = tmp;
        vo = tmp + n / 2;
        for (k = 0; k < n / 2; k++) {
            ve[k] = v[2 * k];
            vo[k] = v[2 * k + 1];
        }
        um_fft(ve, n / 2, v); /* FFT on even-indexed elements of v[] */
        um_fft(vo, n / 2, v); /* FFT on odd-indexed elements of v[] */
        for (m = 0; m < n / 2; m++) {
            w.real = cos(2 * UM_PI * m / (double)n);
            w.imag = -sin(2 * UM_PI * m / (double)n);
            z.real = w.real * vo[m].real - w.imag * vo[m].imag; /* Re(w*vo[m]) */
            z.imag = w.real * vo[m].imag + w.imag * vo[m].real; /* Im(w*vo[m]) */
            v[m].real = ve[m].real + z.real;
            v[m].imag = ve[m].imag + z.imag;
            v[m + n / 2].real = ve[m].real - z.real;
            v[m + n / 2].imag = ve[m].imag - z.imag;
        }
    }
}

/*
    ifft(v,N):
    Inverse Factored discrete Fourier Transform.
    From https://www.math.wustl.edu/~victor/mfmm/fourier/fft.c
    [0] If N==1 then return.
    [1] For k = 0 to N/2-1, let ve[k] = v[2*k]
    [2] Compute ifft(ve, N/2);
    [3] For k = 0 to N/2-1, let vo[k] = v[2*k+1]
    [4] Compute ifft(vo, N/2);
    [5] For m = 0 to N/2-1, do [6] through [9]
    [6]   Let w.re = cos(2*M_PI*m/N)
    [7]   Let w.im = sin(2*M_PI*m/N)
    [8]   Let v[m] = ve[m] + w*vo[m]
    [9]   Let v[m+N/2] = ve[m] - w*vo[m]
*/
void
um_ifft(um_complex *v, int n, um_complex *tmp)
{
    if (n > 1) { /* otherwise, do nothing and return */
        int k, m;
        um_complex z, w, *vo, *ve;
        ve = tmp;
        vo = tmp + n / 2;
        for (k = 0; k < n / 2; k++) {
            ve[k] = v[2 * k];
            vo[k] = v[2 * k + 1];
        }
        um_ifft(ve, n / 2, v); /* FFT on even-indexed elements of v[] */
        um_ifft(vo, n / 2, v); /* FFT on odd-indexed elements of v[] */
        for (m = 0; m < n / 2; m++) {
            w.real = cos(2 * UM_PI * m / (double)n);
            w.imag = sin(2 * UM_PI * m / (double)n);
            z.real = w.real * vo[m].real - w.imag * vo[m].imag; /* Re(w*vo[m]) */
            z.imag = w.real * vo[m].imag + w.imag * vo[m].real; /* Im(w*vo[m]) */
            v[m].real = ve[m].real + z.real;
            v[m].imag = ve[m].imag + z.imag;
            v[m + n / 2].real = ve[m].real - z.real;
            v[m + n / 2].imag = ve[m].imag - z.imag;
        }
    }
}

#if 0 /* Alternative implementation of FFT. TODO: Compare with current. */
#include <complex.h>
typedef float complex cplx;
static void um__fft(cplx buf[], cplx out[], int n, int step) {
    if (step < n) {
        um__fft(out, buf, n, step * 2);
        um__fft(out + step, buf + step, n, step * 2);

        for (int i = 0; i < n; i += 2 * step) {
            cplx t = cexpf(-I * M_PI * i / n) * out[i + step];
            buf[i / 2] = out[i] + t;
            buf[(i + n) / 2] = out[i] - t;
        }
    }
}

/* Fast fourier transform from
 * https://rosettacode.org/wiki/Fast_Fourier_transform#C.2B.2B
 *
 * n is expected to be power of 2.
 */
void rosetta_fft(cplx buf[], int n) {
    cplx out[n];
    for (int i = 0; i < n; i++)
        out[i] = buf[i];

    um__fft(buf, out, n, 1);
}
#endif

static void
um_load_defaults(umugu_ctx *ctx)
{
    ctx->io.log("Could not load config from file, setting defaults...\n");
    ctx->pipeline.sig.samples.channel_count = 1;
    ctx->pipeline.sig.sample_rate = UM_DEFAULT_SAMPLE_RATE;
    ctx->pipeline.sig.format = UM_DEFAULT_SAMPLE_FORMAT;

    ctx->io.backend.channel_count = UM_DEFAULT_CHANNELS;
    ctx->io.backend.sample_rate = UM_DEFAULT_SAMPLE_RATE;
    ctx->io.backend.format = UM_DEFAULT_SAMPLE_FORMAT;
    ctx->io.backend.interleaved_channels = UM_DEFAULT_INTERLEAVED_CHANNELS;
    ctx->io.backend.audio_input = UM_DEFAULT_AUDIO_IN;
    ctx->io.backend.audio_output = UM_DEFAULT_AUDIO_OUT;
    ctx->io.backend.audio_callback = umugu_process;

    /* TODO: Remove io.out_audio. Use umugu_produce_signal params or umugu_io ringbuffer. */
    ctx->io.out_audio.samples.channel_count = UM_DEFAULT_CHANNELS;
    ctx->io.out_audio.sample_rate = UM_DEFAULT_SAMPLE_RATE;
    ctx->io.out_audio.format = UM_DEFAULT_SAMPLE_FORMAT;

    ctx->io.in_audio.samples.channel_count = UM_DEFAULT_AUDIO_IN;
    ctx->io.in_audio.interleaved_channels = UM_DEFAULT_INTERLEAVED_CHANNELS;
    ctx->io.out_audio.samples.channel_count = UM_DEFAULT_AUDIO_OUT;
    ctx->io.out_audio.interleaved_channels = UM_DEFAULT_INTERLEAVED_CHANNELS;
    /* remove until here */

    ctx->ppln_iterations = 0;
    ctx->ppln_it_allocated = 0;
}

/**
 * [key, value] Map for temporal storage of config variables during parse.
 *   It is a hash table of fixed_str32 key and u16 value (indices into the string storage).
 */
struct um_confmap {
    umugu_name keys[UMUGU_CONFIG_MAP_SIZE];
    uint16_t value_idx[UMUGU_CONFIG_MAP_SIZE]; //!< Index (offset) of storage where the value is.
    char storage[UMUGU_CONFIG_MAP_SIZE * UMUGU_CONFIG_VALUE_LEN]; //!< Tightly packed values.
    uint16_t storage_tail;
    uint16_t count;
};

static inline um_confmap
um_confmap_create_zeroed(void)
{
    return (um_confmap){
        .keys = {0}, .value_idx = {0}, .storage = {0}, .storage_tail = 0, .count = 0};
}

static inline int
um_confmap_get_free_slot(um_confmap *cm, const umugu_name *key)
{
    uint64_t hash = um_name_hash(key);
    int i = hash % UMUGU_CONFIG_MAP_SIZE;
    while (!um_name_empty(&cm->keys[i])) {
        i = (i + 1) % UMUGU_CONFIG_MAP_SIZE;
    }
    return i;
}

static int
um_confmap_add_entry(um_confmap *cm, int idx, const umugu_name *key, const char *value, size_t len)
{
    const int arena_it = cm->storage_tail;
    UMUGU_ASSERT((arena_it + len + 1) < (UMUGU_CONFIG_MAP_SIZE * UMUGU_CONFIG_VALUE_LEN));
    if ((arena_it + len + 1) >= (UMUGU_CONFIG_MAP_SIZE * UMUGU_CONFIG_VALUE_LEN)) {
        UMUGU_ASSERT(0);
        return UMUGU_ERR_FULL_STORAGE;
    }
    cm->keys[idx] = *key;
    cm->value_idx[idx] = arena_it;
    memcpy(cm->storage + arena_it, value, len);
    cm->storage_tail += len;
    cm->storage[cm->storage_tail++] = 0;
    cm->count++;
    return idx;
}

static int
um_confmap_insert(um_confmap *cm, const umugu_name *key, const char *value, size_t len)
{
    UMUGU_ASSERT(
        cm->count < (UMUGU_CONFIG_MAP_SIZE - UMUGU_CONFIG_MAP_SIZE / 5) &&
        "The confmap is 80%% full, consider increasing its size since "
        "it is implemented as a hash table with open addressing.\n");
    if (cm->count >= UMUGU_CONFIG_MAP_SIZE) {
        return UMUGU_ERR_FULL_TABLE;
    }

    return um_confmap_add_entry(cm, um_confmap_get_free_slot(cm, key), key, value, len);
}

static const char *
um_confmap_get(const um_confmap *cm, const umugu_name *key)
{
    uint64_t hash = um_name_hash(key);
    int i = hash % UMUGU_CONFIG_MAP_SIZE;
    while (!um_name_equals(key, &cm->keys[i])) {
        if (um_name_empty(&cm->keys[i])) {
            return NULL;
        }
        i = (i + 1) % UMUGU_CONFIG_MAP_SIZE;
    }
    return &cm->storage[cm->value_idx[i]];
}

static const char UM_CONFIG_START_LN[] = "; UMUGU Config start.\n";
static const char UM_CONFIG_END_LN[] = "; UMUGU Config end.\n";
static const umugu_name UM_CONFMAP_FALLBACK_WAV_FILE = {.str = "FallbackWavFile"};
static const umugu_name UM_CONFMAP_FALLBACK_SF2_FILE = {.str = "FallbackSoundFont2File"};
static const umugu_name UM_CONFMAP_FALLBACK_MIDI_DEVICE = {.str = "FallbackMidiDevice"};
static const umugu_name UM_CONFMAP_NUM_CHANNELS = {.str = "NumChannels"};
static const umugu_name UM_CONFMAP_SAMPLE_RATE = {.str = "SampleRate"};
static const umugu_name UM_CONFMAP_SAMPLE_FORMAT = {.str = "SampleFormat"};

static int
um_load_config(umugu_ctx *ctx, const char *filename)
{
    char *buffer = (char *)ctx->arena_tail;
    ptrdiff_t remaining_mem = (ctx->arena_head + ctx->arena_capacity) - ctx->arena_tail;
    UMUGU_ASSERT(remaining_mem > 0);
    size_t required_size = ctx->io.file_read(filename, buffer, remaining_mem);
    if (required_size > (size_t)remaining_mem) {
        buffer = um_talloc(ctx, required_size);
        if (ctx->io.file_read(filename, buffer, required_size) != required_size) {
            UMUGU_ASSERT(0 && "Wtf happened?");
            return UMUGU_ERR_MEM;
        }
    } else {
        ctx->arena_tail += required_size;
    }

    um_confmap configs = um_confmap_create_zeroed();
    char *it = buffer;
    if (memcmp(it, UM_CONFIG_START_LN, sizeof(UM_CONFIG_START_LN) - 1)) {
        UMUGU_ASSERT(0 && "Config format error.");
        return UMUGU_ERR_FILE;
    }

    it += sizeof(UM_CONFIG_START_LN) - 1;
    while (*it && ((required_size - (it - buffer)) >= sizeof(UM_CONFIG_END_LN))) {
        if (!memcmp(it, UM_CONFIG_END_LN, sizeof(UM_CONFIG_END_LN) - 1)) {
            break;
        }
        char *field_end = strpbrk(it, " =\t\n\r\0");
        register char *tmp = strpbrk(field_end, "=\n\0");
        if (*tmp == '\n') {
            continue;
        } else if (*tmp == '\0') {
            break;
        }

        UMUGU_ASSERT(*field_end == '=');
        umugu_name field;
        um_name_clear(&field);
        memcpy(field.str, it, um_mini(field_end - it, UMUGU_NAME_LEN - 1));
        it = tmp + 1;
        char *value_end = strpbrk(it, " =\t\n\r\0");
        UMUGU_ASSERT(*value_end == '\n');
        if (um_confmap_insert(&configs, &field, it, value_end - it) < 0) {
            UMUGU_ASSERT(0 && "Error inserting config in map.");
            return UMUGU_ERR_CONFIG;
        }
        it = value_end + 1;
    }

    // FallbackAudioFile
    umugu_name key = UM_CONFMAP_FALLBACK_WAV_FILE;
    strncpy(ctx->fallback_wav_file, um_confmap_get(&configs, &key), UMUGU_PATH_LEN);

    // FallbackAudioFile
    key = UM_CONFMAP_FALLBACK_SF2_FILE;
    strncpy(ctx->fallback_soundfont2_file, um_confmap_get(&configs, &key), UMUGU_PATH_LEN);

    // FallbackMidiDevice
    key = UM_CONFMAP_FALLBACK_MIDI_DEVICE;
    strncpy(ctx->fallback_midi_device, um_confmap_get(&configs, &key), UMUGU_PATH_LEN);

    // NumChannels
    key = UM_CONFMAP_NUM_CHANNELS;
    const char *value = um_confmap_get(&configs, &key);
    if (value) {
        ctx->pipeline.sig.samples.channel_count = atoi(value);
    }

    // SampleRate
    key = UM_CONFMAP_SAMPLE_RATE;
    value = um_confmap_get(&configs, &key);
    if (value) {
        ctx->pipeline.sig.sample_rate = atoi(value);
    }

    // SampleFormat
    key = UM_CONFMAP_SAMPLE_FORMAT;
    value = um_confmap_get(&configs, &key);
    if (value) {
        ctx->pipeline.sig.format = atoi(value);
    }

    return UMUGU_SUCCESS;
}

/* ## BUILT-IN NODES INFO ## */

/*  OSCILLATOR  */
const umugu_attrib_info um_oscil_attribs[] = {
    {.name = {.str = "Frequency"},
     .offset_bytes = offsetof(um_oscil, osc) + offsetof(um_oscillator, freq),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .misc.rangef.min = 1,
     .misc.rangef.max = 8372},
    {.name = {.str = "Waveform"},
     .offset_bytes = offsetof(um_oscil, waveform),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .misc.rangei.min = 0,
     .misc.rangei.max = UMUGU_WAVEFORM_COUNT}};
const int um_oscil_size = (int)sizeof(um_oscil);
const int um_oscil_attrib_count = UM_ARRAY_SIZE(um_oscil_attribs);

/*  WAV FILE PLAYER  */
const umugu_attrib_info um_wavplayer_attribs[] = {
    {.name = {.str = "Channels"},
     .offset_bytes =
         offsetof(um_wavplayer, wav) + offsetof(umugu_signal, samples) +
         offsetof(umugu_samples, channel_count),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .flags = UMUGU_ATTR_RDONLY},
    {.name = {.str = "Sample rate"},
     .offset_bytes = offsetof(um_wavplayer, wav) + offsetof(umugu_signal, sample_rate),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .flags = UMUGU_ATTR_RDONLY},
    {.name = {.str = "Sample data type"},
     .offset_bytes = offsetof(um_wavplayer, wav) + offsetof(umugu_signal, format),
     .type = UMUGU_TYPE_INT16,
     .count = 1,
     .flags = UMUGU_ATTR_RDONLY},
    {.name = {.str = "Filename"},
     .offset_bytes = offsetof(um_wavplayer, filename),
     .type = UMUGU_TYPE_TEXT,
     .count = UMUGU_PATH_LEN}};
const int um_wavplayer_size = (int)sizeof(um_wavplayer);
const int um_wavplayer_attrib_count = UM_ARRAY_SIZE(um_wavplayer_attribs);

/*  AMPLITUDE  */
const umugu_attrib_info um_amplitude_attribs[] = {
    {.name = {.str = "Multiplier"},
     .offset_bytes = offsetof(um_amplitude, multiplier),
     .type = UMUGU_TYPE_FLOAT,
     .count = 1,
     .misc.rangef.min = 0.0f,
     .misc.rangef.max = 5.0f}};
const int um_amplitude_size = (int)sizeof(um_amplitude);
const int um_amplitude_attrib_count = UM_ARRAY_SIZE(um_amplitude_attribs);

/*  LIMITER  */
const umugu_attrib_info um_limiter_attribs[] = {
    {.name = {.str = "Min"},
     .offset_bytes = offsetof(um_limiter, min),
     .type = UMUGU_TYPE_FLOAT,
     .count = 1,
     .misc.rangef.min = -5.0f,
     .misc.rangef.max = 5.0f},
    {.name = {.str = "Max"},
     .offset_bytes = offsetof(um_limiter, max),
     .type = UMUGU_TYPE_FLOAT,
     .count = 1,
     .misc.rangef.min = -5.0f,
     .misc.rangef.max = 5.0f}};
const int um_limiter_size = (int)sizeof(um_limiter);
const int um_limiter_attrib_count = UM_ARRAY_SIZE(um_limiter_attribs);

/*  MIXER  */
const umugu_attrib_info um_mixer_attribs[] = {
    {.name = {.str = "InputCount"},
     .offset_bytes = offsetof(um_mixer, input_count),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .misc.rangei.min = 0,
     .misc.rangei.max = UMUGU_MIXER_MAX_INPUTS},
    {.name = {.str = "ExtraInputPipeNodeIdx"},
     .offset_bytes = offsetof(um_mixer, extra_pipe_in_node_idx),
     .type = UMUGU_TYPE_INT16,
     .count = UMUGU_MIXER_MAX_INPUTS,
     .misc.rangei.min = 0,
     .misc.rangei.max = 9999},
    {.name = {.str = "ExtraInputPipeChannel"},
     .offset_bytes = offsetof(um_mixer, extra_pipe_in_channel),
     .type = UMUGU_TYPE_INT8,
     .count = UMUGU_MIXER_MAX_INPUTS,
     .misc.rangei.min = 0,
     .misc.rangei.max = UMUGU_MIXER_MAX_INPUTS}};
const int um_mixer_size = (int)sizeof(um_mixer);
const int um_mixer_attrib_count = UM_ARRAY_SIZE(um_mixer_attribs);

/*  OUTPUT  */
const umugu_attrib_info um_output_attribs[] = {
    {.name = {.str = "Input node"},
     .offset_bytes = offsetof(umugu_node, prev_node),
     .type = UMUGU_TYPE_INT16,
     .count = 0,
     .flags = UMUGU_ATTR_RDONLY,
     .misc.rangei.min = -1,
     .misc.rangei.max = -1}};
const int um_output_size = (int)sizeof(um_output);
const int um_output_attrib_count = UM_ARRAY_SIZE(um_output_attribs);

static const umugu_node_type_info um_builtin_ninfo[] = {
    {.name = {"Oscillator"},
     .size_bytes = um_oscil_size,
     .attrib_count = um_oscil_attrib_count,
     .getfn = um_oscil_getfn,
     .attribs = um_oscil_attribs,
     .plug_handle = NULL},

    {.name = {"WavFilePlayer"},
     .size_bytes = um_wavplayer_size,
     .attrib_count = um_wavplayer_attrib_count,
     .getfn = um_wavplayer_getfn,
     .attribs = um_wavplayer_attribs,
     .plug_handle = NULL},

    {.name = {"Mixer"},
     .size_bytes = um_mixer_size,
     .attrib_count = um_mixer_attrib_count,
     .getfn = um_mixer_getfn,
     .attribs = um_mixer_attribs,
     .plug_handle = NULL},

    {.name = {"Amplitude"},
     .size_bytes = um_amplitude_size,
     .attrib_count = um_amplitude_attrib_count,
     .getfn = um_amplitude_getfn,
     .attribs = um_amplitude_attribs,
     .plug_handle = NULL},

    {.name = {"Limiter"},
     .size_bytes = um_limiter_size,
     .attrib_count = um_limiter_attrib_count,
     .getfn = um_limiter_getfn,
     .attribs = um_limiter_attribs,
     .plug_handle = NULL},

    {.name = {"Output"},
     .size_bytes = um_output_size,
     .attrib_count = um_output_attrib_count,
     .getfn = um_output_getfn,
     .attribs = um_output_attribs,
     .plug_handle = NULL},
};

static const umugu_node_type_info *
um_node_info_builtin_find(const umugu_name *name)
{
    enum { UM_BUILTIN_NODE_COUNT = UM_ARRAY_SIZE(um_builtin_ninfo) };
    for (int i = 0; i < UM_BUILTIN_NODE_COUNT; ++i) {
        if (um_name_equals(&um_builtin_ninfo[i].name, name)) {
            return um_builtin_ninfo + i;
        }
    }
    return NULL;
}

// xxHash implementation
static inline uint64_t
um_hash_avalanche(uint64_t hash)
{
    static const uint64_t PRIME = 0x165667919E3779F9ULL;
    hash = hash ^ (hash >> 37);
    hash *= PRIME;
    return hash ^ (hash >> 32);
}

static inline uint64_t
um_hash_mul128_fold64(uint64_t lhs, uint64_t rhs)
{
    __uint128_t const v = (__uint128_t)lhs * (__uint128_t)rhs;
    return v ^ (v >> 64);
}

static inline uint64_t
um_hash_mix16(const uint8_t *data, const uint8_t *sec, uint64_t seed)
{
    uint64_t lo = *(uint64_t *)data;
    uint64_t hi = *(uint64_t *)(data + 8);
    return um_hash_mul128_fold64(
        lo ^ (*(uint64_t *)sec + seed), hi ^ (*(uint64_t *)(sec + 8) - seed));
}

uint64_t
um_name_hash(const umugu_name *name)
{
    static const uint64_t PRIME = 0x9E3779B185EBCA87ULL;
    static const uint8_t SECRET[] = {
        0xb8, 0xfe, 0x6c, 0x39, 0x23, 0xa4, 0x4b, 0xbe, 0x7c, 0x01, 0x81,
        0x2c, 0xf7, 0x21, 0xad, 0x1c, 0xde, 0xd4, 0x6d, 0xe9, 0x83, 0x90,
        0x97, 0xdb, 0x72, 0x40, 0xa4, 0xa4, 0xb7, 0xb3, 0x67, 0x1f};

    const uint8_t *data = (uint8_t *)name->str;
    uint64_t acc = 32 * PRIME;
    acc += um_hash_mix16(data, SECRET, 0);
    acc += um_hash_mix16((data + 16), SECRET + 16, 0);
    return um_hash_avalanche(acc);
}
