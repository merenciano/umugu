#include "umugu.h"

#include "controller.h"
#include "umugu_internal.h"

#include <assert.h>
#include <dlfcn.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define UM_ARRAY_COUNT(ARR) ((int32_t)(sizeof(ARR) / sizeof(*(ARR))))

#define UM_PTR(PTR, OFFSET_BYTES)                                              \
    ((void *)((char *)((void *)(PTR)) + (OFFSET_BYTES)))

#define UM_DIST_BYTES(HEAD_PTR, TAIL_PTR)                                      \
    (((char *)((void *)(TAIL_PTR))) - ((char *)((void *)(HEAD_PTR))))

static umugu_ctx g_default_ctx; /* Relying on zero-init from static memory. */
static umugu_ctx *g_ctx = &g_default_ctx;

static inline int
um_name_equals(const umugu_name *a, const umugu_name *b)
{
    return !memcmp(a, b, UMUGU_NAME_LEN);
}

const umugu_node_info *um_node_info_builtin_find(const umugu_name *name);

const umugu_node_info *
umugu_node_info_load(const umugu_name *name)
{
    /* Check if the node info is already loaded. */
    for (int i = 0; i < g_ctx->nodes_info_next; ++i) {
        if (um_name_equals(&g_ctx->nodes_info[i].name, name)) {
            return g_ctx->nodes_info + i;
        }
    }

    /* Look for it in the built-in nodes. */
    const umugu_node_info *bi_info = um_node_info_builtin_find(name);
    if (bi_info) {
        /* Add the node info to the current context. */
        assert(g_ctx->nodes_info_next < UMUGU_DEFAULT_NODE_INFO_CAPACITY);
        umugu_node_info *info = &g_ctx->nodes_info[g_ctx->nodes_info_next++];
        memcpy(info, bi_info, sizeof(umugu_node_info));
#ifdef UMUGU_VERBOSE
        g_ctx->io.log("Built-in node %s loaded.\n", name->str);
#endif
        return info;
    }

    /* Check if there's a plug with that name and load it. */
    int ret = umugu_plug(name);
    if (ret >= 0) {
        return g_ctx->nodes_info + ret;
    }

    /* Node info not found. */
    assert(ret == UMUGU_ERR_PLUG);
    g_ctx->io.log("Node type %s not found.\n", name->str);
    return NULL;
}

int /* TODO: Move umugu_set_arena and pipeline loading into this func. */
umugu_init(void)
{
    g_ctx->state = UMUGU_STATE_INITIALIZING;
    um_nanosec init_time = um_time_now();
    umugu_ctrl_init();
    g_ctx->init_time_ns = um_time_elapsed(init_time);
    g_ctx->state = UMUGU_STATE_READY;
#ifdef UMUGU_VERBOSE
    g_ctx->io.log("Umugu initialized in %ldns\n", g_ctx->init_time_ns);
#endif
    return UMUGU_SUCCESS;
}

int
umugu_produce_signal(void)
{
    g_ctx->state = UMUGU_STATE_REALTIME_PROCESSING;
    /* TODO: Ringbuffer and proper implementation of separate threads with
            unsync callbacks for the midi controller and the audio output
            for example. Right now the init is in this func because
            umugu_produce_signal is called from the audio backend thread,
            and some plugin nodes require to call some funcions from the
            thread where the objects were created, so calling the init from
            the audio callback ensures that. */
    if (!g_ctx->pipeline_iteration) {
        for (int i = 0; i < g_ctx->pipeline.node_count; ++i) {
            umugu_node *node = g_ctx->pipeline.nodes[i];
            int err = umugu_node_dispatch(node, UMUGU_FN_INIT);
            if (err < UMUGU_SUCCESS) {
                g_ctx->io.log("Error (%d) processing node:\n"
                              "\tIndex: %d.\n\tName: %s\n",
                              err, i, g_ctx->nodes_info[node->type].name);
            }
        }
    }

    g_ctx->pipeline_iteration++;
    g_ctx->arena.alloc_counter = 0;
    umugu_ctrl_update();

    for (int i = 0; i < g_ctx->pipeline.node_count; ++i) {
        umugu_node *node = g_ctx->pipeline.nodes[i];
        int err = umugu_node_dispatch(node, UMUGU_FN_PROCESS);
        if (err < UMUGU_SUCCESS) {
            UMUGU_TRAP();
            g_ctx->io.log("Error (%d) processing node:\n"
                          "\tIndex: %d.\n\tName: %s\n",
                          err, i, g_ctx->nodes_info[node->type].name);
        }
    }

#ifdef UMUGU_VERBOSE
    g_ctx->io.log("TempAlloc bytes in iteration %d: %ld\n",
                  g_ctx->pipeline_iteration, g_ctx->arena.alloc_counter);
#endif
    g_ctx->state = UMUGU_STATE_LISTENING;
    return UMUGU_SUCCESS;
}

void
umugu_set_context(umugu_ctx *new_ctx)
{
    g_ctx = new_ctx;
}

umugu_ctx *
umugu_get_context(void)
{
    return g_ctx;
}

int
umugu_set_arena(void *buffer, size_t bytes)
{
    g_ctx->arena.buffer = buffer;
    g_ctx->arena.capacity = bytes;
    g_ctx->arena.pers_next = buffer;
    g_ctx->arena.temp_next = buffer;

    return UMUGU_SUCCESS;
}

void *
umugu_alloc_pers(size_t bytes)
{
    if (g_ctx->state > UMUGU_STATE_READY) {
        g_ctx->io.log("At this point of the execution, every required"
                      " permanent allocation should have been done yet.\n");

        assert(
            (g_ctx->state != UMUGU_STATE_REALTIME_PROCESSING) &&
            "Permanent allocations are not permitted during pipeline "
            "processing, "
            "since the temporary allocs are heavily used and the new permanent "
            "allocations expand the buffer stealing space from that buffer.");
    }
    umugu_mem_arena *mem = &g_ctx->arena;
    uint8_t *ret = mem->pers_next;
    if ((ret + bytes) > (mem->buffer + mem->capacity)) {
        g_ctx->io.log("Fatal error: Alloc failed. Not enough "
                      "space left in the arena.\n");
        return NULL;
    }
    mem->pers_next += bytes;
    assert(mem->pers_next < (mem->buffer + mem->capacity));
    return ret;
}

void *
umugu_alloc_temp(size_t bytes)
{
    if (g_ctx->state < UMUGU_STATE_READY) {
        g_ctx->io.log(
            "Careful with temporary allocations during"
            " initialization since the next persistent allocation will "
            "steal that region. It is OK for local use but are you sure"
            " you can not use the stack in this situation?\n");
    }

    umugu_mem_arena *mem = &g_ctx->arena;
    uint8_t *ret =
        mem->temp_next > mem->pers_next ? mem->temp_next : mem->pers_next;
    if ((ret + bytes) > (mem->buffer + mem->capacity)) {
        /* Start again: temp memory is used as a circ buffer. */
        ret = mem->pers_next;
        if ((ret + bytes) > (mem->buffer + mem->capacity)) {
            g_ctx->io.log("Fatal error: Alloc failed. Not enough "
                          "space left in the arena.\n");
            return NULL;
        }
    }

    mem->temp_next = ret + bytes;
    mem->alloc_counter += bytes;
    return ret;
}

umugu_sample *
umugu_alloc_signal(umugu_signal *s)
{
    assert(s);
    assert(s->channels > 0 && s->channels < 32 &&
           "Invalid number of channels.");
    s->count = g_ctx->io.out_audio.count;
    assert(s->count > 0);
    s->samples =
        umugu_alloc_temp(s->count * sizeof(umugu_sample) * s->channels);
    return s->samples;
}

void *
umugu_alloc_generic_signal(umugu_generic_signal *s)
{
    assert(s);
    s->count = g_ctx->io.out_audio.count;
    assert(s->count > 0);
    s->sample_data =
        umugu_alloc_temp(s->count * um_sizeof(s->type) * s->channels);
    return s->sample_data;
}

int
umugu_node_dispatch(umugu_node *node, umugu_fn fn)
{
    assert(node->type >= 0 && "Node info index can not be negative.");
    assert(g_ctx->nodes_info_next > node->type && "Node info not loaded.");
    umugu_node_fn func = g_ctx->nodes_info[node->type].getfn(fn);
    return func ? func(node) : UMUGU_ERR_NULL;
}

int
umugu_plug(const umugu_name *name)
{
    char buf[1024];
    snprintf(buf, 1024, "lib%s.so", name->str);

    assert(g_ctx->nodes_info_next < UMUGU_DEFAULT_NODE_INFO_CAPACITY);
    void *hnd = dlopen(buf, RTLD_NOW);
    if (!hnd) {
        g_ctx->io.log("Can't load plug: dlopen(%s) failed.", buf);
        return UMUGU_ERR_PLUG;
    }

    umugu_node_info *info = &g_ctx->nodes_info[g_ctx->nodes_info_next++];
    info->name = *name;
    *(void **)&info->getfn = dlsym(hnd, "getfn");
    info->size_bytes = *(int32_t *)dlsym(hnd, "size");
    info->vars = *(umugu_var_info **)dlsym(hnd, "vars");
    info->var_count = *(int32_t *)dlsym(hnd, "var_count");
    info->plug_handle = hnd;

#ifdef UMUGU_VERBOSE
    g_ctx->io.log("Node plugged successfuly: %s\n", name->str);
#endif
    return g_ctx->nodes_info_next - 1;
}

void
umugu_unplug(umugu_node_info *info)
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
um_oscillator_sine(um_oscillator *self, umugu_signal *sig)
{
    umugu_ctx *ctx = umugu_get_context();
    const float k_twopi = 2.0f * M_PI;
    const float w = self->freq * k_twopi / ctx->config.sample_rate;
    const int count = sig->count;
    sig->channels = 1;

    float b1 = 2.0f * cosf(w);
    sig->samples[0] = sinf(self->phase);
    sig->samples[1] = sinf(self->phase + w);
    for (int i = 2; i < count; ++i) {
        sig->samples[i] = b1 * sig->samples[i - 1] - sig->samples[i - 2];
    }

    self->phase = fmodf(self->phase + count * w, 2.0f * M_PI);
}

void
um_oscillator_sawsin(um_oscillator *self, umugu_signal *sig)
{
    static const float k_twopi = 2.0f * M_PI;
    const int count = sig->count;
    sig->channels = 1;
    for (int i = 0; i < count; ++i) {
        float t = fmodf(self->phase / k_twopi, 1.0f);
        if (t > 0.5f) {
            sig->samples[i] = -sinf(self->phase);
        } else {
            sig->samples[i] = 2.0f * t - 1.0f;
        }
        self->phase += self->freq;
    }
}

void
um_oscillator_saw(um_oscillator *self, umugu_signal *sig)
{
    umugu_ctx *ctx = umugu_get_context();
    const int count = sig->count;
    const float sr = ctx->config.sample_rate;
    const float hsr = sr * 0.5f;
    sig->channels = 1;
    for (int i = 0; i < count; ++i) {
        float value = self->phase / hsr;
        sig->samples[i] = value > 1.0f ? value - 2.0f : value;
        self->phase = um_oscillator_phase(self->phase, self->freq, sr);
        self->phase += self->freq;
        if (self->phase >= ctx->config.sample_rate) {
            self->phase -= ctx->config.sample_rate;
        }
    }
}

void
um_oscillator_triangle(um_oscillator *self, umugu_signal *sig)
{
    umugu_ctx *ctx = umugu_get_context();
    const float rate = ctx->config.sample_rate;
    const float rqsr = 1.0f / (rate / 4.0f);
    const int count = sig->count;
    sig->channels = 1;
    static const float k_lut[] = {0.0f, 1.0f, 0.0f, -1.0f, 0.0f};
    for (int i = 0; i < count; ++i) {
        float t = self->phase * rqsr;
        int ipart = (int)t;
        sig->samples[i] = um_lerp(k_lut[ipart], k_lut[ipart + 1], t - ipart);

        self->phase += self->freq;
        if (self->phase >= ctx->config.sample_rate) {
            self->phase -= ctx->config.sample_rate;
        }
    }
}

void
um_oscillator_square(um_oscillator *self, umugu_signal *sig)
{
    umugu_ctx *ctx = umugu_get_context();
    const float k_twopi = 2.0f * M_PI;
    const float w = self->freq * k_twopi / ctx->config.sample_rate;
    const int count = sig->count;
    sig->channels = 1;
    while (self->phase >= k_twopi) {
        self->phase -= k_twopi;
    }

    for (int i = 0; i < count; ++i) {
        sig->samples[i] = (self->phase / k_twopi) < 0.5f ? 1.0f : -1.0f;
        float next_phase = self->phase + w;
        self->phase = next_phase < k_twopi ? next_phase : next_phase - k_twopi;
    }

    self->phase = fmodf(self->phase + count * w, 2.0f * M_PI);
}

void
um_noisegen_white(um_noisegen *gen, umugu_signal *sig)
{
    assert(sig && (sig->count > 0) && sig->channels == 1);
    assert(gen);
    static const int k_x1 = 0x67452301;
    static const int k_x2 = 0xefcdab89;

    if (!gen->x1) {
        gen->x1 = k_x1;
    }

    if (!gen->x2) {
        gen->x2 = k_x2;
    }

    /* TODO: temporal assert, still deciding if this kind of funcs should
             allocate the signal buffer. */
    assert(sig->samples);
#if 0
    if (!sig->samples) {
        umugu_alloc_signal(sig);
    }
#endif

    const int count = sig->count;
    for (int i = 0; i < count; ++i) {
        gen->x1 ^= gen->x2;
        sig->samples[i] = gen->x2 * (2.0f / (float)0xffffffff);
        gen->x2 += gen->x1;
    }
}

float
um_note_freq(int note_index)
{
    const float um_note_freq[] = {261.63f, 277.18f, 293.66f, 311.13f,
                                  329.63f, 349.23f, 369.99f, 392.0f,
                                  415.3f,  440.0f,  466.16f, 493.88f};
    const float um_note_oct_mul[] = {0.03125f, 0.0625f, 0.125f, 0.25f,
                                     0.5f,     1.0f,    2.0f,   4.0f,
                                     8.0f,     16.0f,   32.0f};
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
            w.real = cos(2 * M_PI * m / (double)n);
            w.imag = -sin(2 * M_PI * m / (double)n);
            z.real =
                w.real * vo[m].real - w.imag * vo[m].imag; /* Re(w*vo[m]) */
            z.imag =
                w.real * vo[m].imag + w.imag * vo[m].real; /* Im(w*vo[m]) */
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
            w.real = cos(2 * M_PI * m / (double)n);
            w.imag = sin(2 * M_PI * m / (double)n);
            z.real =
                w.real * vo[m].real - w.imag * vo[m].imag; /* Re(w*vo[m]) */
            z.imag =
                w.real * vo[m].imag + w.imag * vo[m].real; /* Im(w*vo[m]) */
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

/* ## BUILT-IN NODES INFO ## */
const umugu_var_info um__oscope_vars[] = {
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
     .misc.rangei.max = UMUGU_WAVE_COUNT}};
const int32_t um__oscope_size = (int32_t)sizeof(um_oscil);
const int32_t um__oscope_var_count = UM_ARRAY_COUNT(um__oscope_vars);

const umugu_var_info um__wavplayer_vars[] = {
    {.name = {.str = "Channels"},
     .offset_bytes =
         offsetof(um_wavplayer, wav) + offsetof(umugu_generic_signal, channels),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .flags = UMUGU_VAR_RDONLY},
    {.name = {.str = "Sample rate"},
     .offset_bytes =
         offsetof(um_wavplayer, wav) + offsetof(umugu_generic_signal, rate),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .flags = UMUGU_VAR_RDONLY},
    {.name = {.str = "Sample data type"},
     .offset_bytes =
         offsetof(um_wavplayer, wav) + offsetof(umugu_generic_signal, type),
     .type = UMUGU_TYPE_INT16,
     .count = 1,
     .flags = UMUGU_VAR_RDONLY},
    {.name = {.str = "Filename"},
     .offset_bytes = offsetof(um_wavplayer, filename),
     .type = UMUGU_TYPE_TEXT,
     .count = UMUGU_PATH_LEN}};
const int32_t um__wavplayer_size = (int32_t)sizeof(um_wavplayer);
const int32_t um__wavplayer_var_count = UM_ARRAY_COUNT(um__wavplayer_vars);

const umugu_var_info um__amplitude_vars[] = {
    {.name = {.str = "Multiplier"},
     .offset_bytes = offsetof(um_amplitude, multiplier),
     .type = UMUGU_TYPE_FLOAT,
     .count = 1,
     .misc.rangef.min = 0.0f,
     .misc.rangef.max = 5.0f}};
const int32_t um__amplitude_size = (int32_t)sizeof(um_amplitude);
const int32_t um__amplitude_var_count = UM_ARRAY_COUNT(um__amplitude_vars);

const umugu_var_info um__limiter_vars[] = {
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
const int32_t um__limiter_size = (int32_t)sizeof(um_limiter);
const int32_t um__limiter_var_count = UM_ARRAY_COUNT(um__limiter_vars);

const umugu_var_info um__mixer_vars[] = {
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
const int32_t um__mixer_size = (int32_t)sizeof(um_mixer);
const int32_t um__mixer_var_count = UM_ARRAY_COUNT(um__mixer_vars);

const umugu_var_info um__piano_vars[] = {
    {.name = {.str = "Waveform"},
     .offset_bytes = offsetof(um_piano, waveform),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .misc.rangei.min = 0,
     .misc.rangei.max = UMUGU_WAVE_COUNT}};
const int32_t um__piano_size = (int32_t)sizeof(um_piano);
const int32_t um__piano_var_count = UM_ARRAY_COUNT(um__piano_vars);

const umugu_var_info um__output_vars[] = {
    {.name = {.str = "Input node"},
     .offset_bytes = offsetof(umugu_node, prev_node),
     .type = UMUGU_TYPE_INT16,
     .count = 0,
     .flags = UMUGU_VAR_RDONLY,
     .misc.rangei.min = -1,
     .misc.rangei.max = -1}};
const int32_t um__output_size = (int32_t)sizeof(um_output);
const int32_t um__output_var_count = UM_ARRAY_COUNT(um__output_vars);

static const umugu_node_info g_um_bi_ninfo[] = {
    {.name = {.str = "Oscilloscope"},
     .size_bytes = um__oscope_size,
     .var_count = um__oscope_var_count,
     .getfn = um_oscil_getfn,
     .vars = um__oscope_vars,
     .plug_handle = NULL},

    {.name = {.str = "WavFilePlayer"},
     .size_bytes = um__wavplayer_size,
     .var_count = um__wavplayer_var_count,
     .getfn = um_wavplayer_getfn,
     .vars = um__wavplayer_vars,
     .plug_handle = NULL},

    {.name = {.str = "Mixer"},
     .size_bytes = um__mixer_size,
     .var_count = um__mixer_var_count,
     .getfn = um_mixer_getfn,
     .vars = um__mixer_vars,
     .plug_handle = NULL},

    {.name = {.str = "Amplitude"},
     .size_bytes = um__amplitude_size,
     .var_count = um__amplitude_var_count,
     .getfn = um_amplitude_getfn,
     .vars = um__amplitude_vars,
     .plug_handle = NULL},

    {.name = {.str = "Limiter"},
     .size_bytes = um__limiter_size,
     .var_count = um__limiter_var_count,
     .getfn = um_limiter_getfn,
     .vars = um__limiter_vars,
     .plug_handle = NULL},

    {.name = {.str = "Piano"},
     .size_bytes = um__piano_size,
     .var_count = um__piano_var_count,
     .getfn = um_piano_getfn,
     .vars = um__piano_vars,
     .plug_handle = NULL},

    {.name = {.str = "Output"},
     .size_bytes = um__output_size,
     .var_count = um__output_var_count,
     .getfn = um_output_getfn,
     .vars = um__output_vars,
     .plug_handle = NULL},
};

const umugu_node_info *
um_node_info_builtin_find(const umugu_name *name)
{
    static const size_t k_builtin_nodes_count = UM_ARRAY_COUNT(g_um_bi_ninfo);
    for (size_t i = 0; i < k_builtin_nodes_count; ++i) {
        if (um_name_equals(&g_um_bi_ninfo[i].name, name)) {
            return g_um_bi_ninfo + i;
        }
    }
    return NULL;
}
