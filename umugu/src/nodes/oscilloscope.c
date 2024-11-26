#include "umugu.h"
#include "umugu_internal.h"

#include <string.h>

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
