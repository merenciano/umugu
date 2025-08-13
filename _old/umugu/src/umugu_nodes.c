#include "umugu.h"
#include "umugu_internal.h"

#include <stdio.h> /* TODO: Use callback funcs */

umugu_node_func um_amplitude_getfn(umugu_fn fn);
umugu_node_func um_limiter_getfn(umugu_fn fn);
umugu_node_func um_mixer_getfn(umugu_fn fn);
umugu_node_func um_oscil_getfn(umugu_fn fn);
umugu_node_func um_wavplayer_getfn(umugu_fn fn);
umugu_node_func um_output_getfn(umugu_fn fn);

/* NODE FUNCTIONS IMPLEMENTATION */
enum { UM_EMPTY_COUNT = 128 };
static const float UM_EMPTY_SAMPLES[UM_EMPTY_COUNT] = {0};

/* AMPLITUDE */
static inline int
um_amplitude_init(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    UM_UNUSED(ctx);
    if (flags & UMUGU_FN_INIT_DEFAULTS) {
        um_amplitude *self = (void *)node;
        self->multiplier = 1.0f;
    }
    node->out_pipe.samples = NULL;
    node->out_pipe.frame_count = 0;
    return UMUGU_SUCCESS;
}

static inline int
um_amplitude_process(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    UM_UNUSED(flags);
    um_amplitude *self = (void *)node;
    const umugu_node *input = um_node_get_input(ctx, node);
    node->out_pipe.channel_count = input->out_pipe.channel_count;
    float *out = um_alloc_samples(ctx, &node->out_pipe);

    const int size = node->out_pipe.frame_count * node->out_pipe.channel_count;
    UMUGU_ASSERT(size > 0);
    for (int i = 0; i < size; ++i) {
        out[i] = input->out_pipe.samples[i] * self->multiplier;
    }

    return UMUGU_SUCCESS;
}

umugu_node_func
um_amplitude_getfn(umugu_fn fn)
{
    switch (fn) {
    case UMUGU_FN_INIT:
        return um_amplitude_init;
    case UMUGU_FN_PROCESS:
        return um_amplitude_process;
    default:
        return NULL;
    }
}

/* LIMITER */
static inline int
um_limiter_init(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    UM_UNUSED(ctx);
    if (flags & UMUGU_FN_INIT_DEFAULTS) {
        um_limiter *self = (void *)node;
        self->min = -1.0f;
        self->max = 1.0f;
    }
    node->out_pipe.samples = NULL;
    node->out_pipe.frame_count = 0;
    return UMUGU_SUCCESS;
}

static inline int
um_limiter_process(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    UM_UNUSED(flags);
    um_limiter *self = (void *)node;

    const umugu_node *input = um_node_get_input(ctx, node);
    node->out_pipe.channel_count = input->out_pipe.channel_count;
    float *out = um_alloc_samples(ctx, &node->out_pipe);

    for (int i = 0; i < node->out_pipe.frame_count * node->out_pipe.channel_count; ++i) {
        out[i] = um_minf(input->out_pipe.samples[i], self->max);
        out[i] = um_maxf(input->out_pipe.samples[i], self->min);
    }

    return UMUGU_SUCCESS;
}

umugu_node_func
um_limiter_getfn(umugu_fn fn)
{
    switch (fn) {
    case UMUGU_FN_INIT:
        return um_limiter_init;
    case UMUGU_FN_PROCESS:
        return um_limiter_process;
    default:
        return NULL;
    }
}

/* MIXER */
static inline int
um_mixer_init(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    UM_UNUSED(ctx);
    if (flags & UMUGU_FN_INIT_DEFAULTS) {
        um_mixer *self = (void *)node;
        self->extra_pipe_in_node_idx[0] = 0;
        self->input_count = 2;
    }
    node->out_pipe.samples = NULL;
    node->out_pipe.frame_count = 0;
    node->out_pipe.channel_count = 1;
    return UMUGU_SUCCESS;
}

static inline int
um_mixer_process(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    um_mixer *self = (void *)node;
    float *out = um_alloc_samples(ctx, &node->out_pipe);
    const int sample_count = node->out_pipe.frame_count;
    int signals = 0;

    const umugu_node *input = um_node_get_input(ctx, node);

    float *in_samples = um_signal_get_channel(&input->out_pipe, node->input_channel);

    ++signals;
    memcpy(out, in_samples, sizeof(float) * sample_count);

    for (int i = 0; i < self->input_count - 1; ++i) {
        umugu_node *in = ctx->pipeline.nodes[self->extra_pipe_in_node_idx[i]];
        if (!in->out_pipe.samples) {
            um_node_dispatch(ctx, node, UMUGU_FN_PROCESS, flags);
            assert(in->out_pipe.samples);
        }

        float *in_samples = um_signal_get_channel(&in->out_pipe, self->extra_pipe_in_channel[i]);
        if (memcmp(
                in_samples, UM_EMPTY_SAMPLES,
                sizeof(float) *
                    (in->out_pipe.frame_count < UM_EMPTY_COUNT
                         ? in->out_pipe.frame_count
                         : UM_EMPTY_COUNT))) {
            ++signals;
        }

        for (int j = 0; j < sample_count; ++j) {
            out[j] += in_samples[j];
        }
    }

    /* Normalize */
    const float inv_count = 1.0f / signals;
    for (int i = 0; i < sample_count; i++) {
        out[i] *= inv_count;
    }

    return UMUGU_SUCCESS;
}

umugu_node_func
um_mixer_getfn(umugu_fn fn)
{
    switch (fn) {
    case UMUGU_FN_INIT:
        return um_mixer_init;
    case UMUGU_FN_PROCESS:
        return um_mixer_process;
    default:
        return NULL;
    }
}

/* OSCILLATOR */
static inline int
um_oscil_init(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    UM_UNUSED(ctx);
    if (flags & UMUGU_FN_INIT_DEFAULTS) {
        um_oscil *self = (void *)node;
        self->waveform = UMUGU_WAVEFORM_SINE;
        self->osc.freq = 440.f;
    }
    node->out_pipe.samples = NULL;
    node->out_pipe.channel_count = 1;
    node->out_pipe.frame_count = 0;
    return UMUGU_SUCCESS;
}

static inline int
um_oscil_process(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    UM_UNUSED(flags);
    um_oscil *self = (void *)node;
    um_alloc_samples(ctx, &node->out_pipe);
    const int sample_rate = ctx->pipeline.sig.sample_rate;

    switch (self->waveform) {
    case UMUGU_WAVEFORM_SINE:
        um_oscillator_sine(&self->osc, &node->out_pipe, sample_rate);
        break;
    case UMUGU_WAVEFORM_SAWSIN:
        um_oscillator_sawsin(&self->osc, &node->out_pipe);
        break;
    case UMUGU_WAVEFORM_SAW:
        um_oscillator_saw(&self->osc, &node->out_pipe, sample_rate);
        break;
    case UMUGU_WAVEFORM_TRIANGLE:
        um_oscillator_triangle(&self->osc, &node->out_pipe, sample_rate);
        break;
    case UMUGU_WAVEFORM_SQUARE:
        um_oscillator_square(&self->osc, &node->out_pipe, sample_rate);
        break;
    case UMUGU_WAVEFORM_WHITE_NOISE:
        um_noisegen_white(&self->noise, &node->out_pipe);
        break;
    }

    return UMUGU_SUCCESS;
}

umugu_node_func
um_oscil_getfn(umugu_fn fn)
{
    switch (fn) {
    case UMUGU_FN_INIT:
        return um_oscil_init;
    case UMUGU_FN_PROCESS:
        return um_oscil_process;
    default:
        return NULL;
    }
}

/* WAV_PLAYER */
static inline void
um_wavplayer_defaults(umugu_ctx *ctx, um_wavplayer *self)
{
    self->node.out_pipe.samples = NULL;
    if (!*ctx->fallback_wav_file) {
        strncpy(self->filename, "../assets/audio/pirri.wav", UMUGU_PATH_LEN); // src Pirri.wav
    } else {
        strncpy(self->filename, ctx->fallback_wav_file, UMUGU_PATH_LEN);
    }
}

static inline int
um_wavplayer_init(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    um_wavplayer *self = (um_wavplayer *)node;

    if (!*self->filename || (flags & UMUGU_FN_INIT_DEFAULTS)) {
        um_wavplayer_defaults(ctx, self);
    }

    self->file_handle = fopen(self->filename, "rb");
    if (!self->file_handle) {
        ctx->io.log("Couldn't open %s\n", self->filename);
        return UMUGU_ERR_FILE;
    }

    /* Advance the file read ptr to where the waveform data starts.
     * While I'm here I'm going to assign the signal format from the header. */
    char header[44];
    fread(header, 44, 1, self->file_handle);
    self->wav.interleaved_channels = true;
    self->wav.samples.frame_count = 0;
    self->wav.samples.channel_count = *(int16_t *)(header + 22);
    self->wav.sample_rate = *(int *)(header + 24);

    const int sample_size = header[34] / 8;
    if (sample_size == 2) {
        self->wav.format = UMUGU_TYPE_INT16;
    } else if (sample_size == 4) {
        self->wav.format = UMUGU_TYPE_FLOAT;
    } else if (sample_size == 1) {
        self->wav.format = UMUGU_TYPE_UINT8;
    } else {
        assert(0 && "Unexpected sample_size.");
    }

    node->out_pipe.channel_count = self->wav.samples.channel_count;
    return UMUGU_SUCCESS;
}

/* TODO: Implement this node type mmapping the wav. */
static inline int
um_wavplayer_process(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    UM_UNUSED(flags);
    um_wavplayer *self = (void *)node;
    assert(
        ((self->wav.samples.channel_count == 1) ||
         ((self->wav.samples.channel_count == 2) && (self->wav.interleaved_channels))) &&
        "Wav file with non-interleaved stereo signals?");

    void *src = um_alloc_signal(ctx, &self->wav);
    fread(
        src,
        um_type_sizeof(self->wav.format) * self->wav.samples.frame_count *
            self->wav.samples.channel_count,
        1, self->file_handle);

    /* TODO: Sample rate conversor. */
    const int sample_rate = ctx->pipeline.sig.sample_rate;
    if (self->wav.sample_rate != sample_rate) {
        ctx->io.log("[ERR] WavPlayer: sample rate must be %d.", sample_rate);
        return UMUGU_ERR_STREAM;
    }

    float *restrict out = um_alloc_samples(ctx, &node->out_pipe);
    const int count = node->out_pipe.frame_count;
    for (int i = 0; i < count; ++i) {
        for (int ch = 0; ch < node->out_pipe.channel_count; ++ch) {
            out[ch * count + i] = um_signal_samplef(&self->wav, i, ch);
            ctx->io.log("{%.2f, %.2f} ");
            if (!ch && !(i % 6)) {
                ctx->io.log("\n");
            }
        }
    }

    return UMUGU_SUCCESS;
}

static inline int
um_wavplayer_destroy(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    UM_UNUSED(ctx), UM_UNUSED(flags);
    um_wavplayer *self = (void *)node;
    if (self->file_handle) {
        fclose(self->file_handle);
        self->file_handle = NULL;
    }
    return UMUGU_SUCCESS;
}

umugu_node_func
um_wavplayer_getfn(umugu_fn fn)
{
    switch (fn) {
    case UMUGU_FN_INIT:
        return um_wavplayer_init;
    case UMUGU_FN_PROCESS:
        return um_wavplayer_process;
    case UMUGU_FN_RELEASE:
        return um_wavplayer_destroy;
    default:
        return NULL;
    }
}

/* OUTPUT */
static inline int
um_output_init(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    UM_UNUSED(ctx), UM_UNUSED(flags);
    node->out_pipe.samples = NULL;
    node->out_pipe.frame_count = 0;
    return UMUGU_SUCCESS;
}

static inline int
um_output_process(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    UM_UNUSED(flags);
    const umugu_node *input = um_node_get_input(ctx, node);

    umugu_signal sigout = ctx->io.out_audio;
    node->out_pipe.samples = sigout.samples.samples;
    /* TODO: SampleRate conversion if the output is different. */
    switch (sigout.format) {
    case UMUGU_TYPE_FLOAT: {
        float *out = sigout.samples.samples;
        if (sigout.interleaved_channels) {
            for (int i = 0; i < sigout.samples.frame_count; ++i) {
                for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i];
                }
            }
        } else {
            for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                for (int i = 0; i < sigout.samples.frame_count; ++i) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i];
                }
            }
        }
        break;
    }
    case UMUGU_TYPE_INT32: {
        int32_t *out = (void *)sigout.samples.samples;
        if (sigout.interleaved_channels) {
            for (int i = 0; i < sigout.samples.frame_count; ++i) {
                for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i] * 2147483648.0f;
                }
            }
        } else {
            for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                for (int i = 0; i < sigout.samples.frame_count; ++i) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i] * 2147483648.0f;
                }
            }
        }
        break;
    }
    case UMUGU_TYPE_INT16: {
        int16_t *out = (void *)sigout.samples.samples;
        if (sigout.interleaved_channels) {
            for (int i = 0; i < sigout.samples.frame_count; ++i) {
                for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i] * 32768.0f;
                }
            }
        } else {
            for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                for (int i = 0; i < sigout.samples.frame_count; ++i) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i] * 32768.0f;
                }
            }
        }
        break;
    }
    case UMUGU_TYPE_INT8: {
        int8_t *out = (void *)sigout.samples.samples;
        if (sigout.interleaved_channels) {
            for (int i = 0; i < sigout.samples.frame_count; ++i) {
                for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i] * 128.0f;
                }
            }
        } else {
            for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                for (int i = 0; i < sigout.samples.frame_count; ++i) {
                    *out++ = um_signal_get_channel(&input->out_pipe, ch)[i] * 128.0f;
                }
            }
        }
        break;
    }
    case UMUGU_TYPE_UINT8: {
        uint8_t *out = (void *)sigout.samples.samples;
        if (sigout.interleaved_channels) {
            for (int i = 0; i < sigout.samples.frame_count; ++i) {
                for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                    *out++ = (um_signal_get_channel(&input->out_pipe, ch)[i] + 1.0f) * 128.0f;
                }
            }
        } else {
            for (int ch = 0; ch < sigout.samples.channel_count; ++ch) {
                for (int i = 0; i < sigout.samples.frame_count; ++i) {
                    *out++ = (um_signal_get_channel(&input->out_pipe, ch)[i] + 1.0f) * 128.0f;
                }
            }
        }
        break;
    }
    default:
        ctx->io.log("[ERR] Output: invalid sample data type.");
        break;
    }

    return UMUGU_SUCCESS;
}

umugu_node_func
um_output_getfn(umugu_fn fn)
{
    switch (fn) {
    case UMUGU_FN_INIT:
        return um_output_init;
    case UMUGU_FN_PROCESS:
        return um_output_process;
    default:
        return NULL;
    }
}
