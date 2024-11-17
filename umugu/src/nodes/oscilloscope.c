#include "umugu.h"
#include "umugu_internal.h"

#include <string.h>

static inline int
um_oscil_init(umugu_node *node)
{
    node->out_pipe.samples = NULL;
    node->out_pipe.channels = 1;
    node->out_pipe.count = 0;
    return UMUGU_SUCCESS;
}

static inline int
um_oscil_defaults(umugu_node *node)
{
    um_oscil *self = (void *)node;
    self->waveform = UMUGU_WAVE_SINE;
    self->osc.freq = 440.f;
    return UMUGU_SUCCESS;
}

static inline int
um_oscil_process(umugu_node *node)
{
    um_node_check_iteration(node);
    um_oscil *self = (void *)node;
    umugu_alloc_signal(&node->out_pipe);

    switch (self->waveform) {
    case UMUGU_WAVE_SINE:
        um_oscillator_sine(&self->osc, &node->out_pipe);
        break;
    case UMUGU_WAVE_SAWSIN:
        um_oscillator_sawsin(&self->osc, &node->out_pipe);
        break;
    case UMUGU_WAVE_SAW:
        um_oscillator_saw(&self->osc, &node->out_pipe);
        break;
    case UMUGU_WAVE_TRIANGLE:
        um_oscillator_triangle(&self->osc, &node->out_pipe);
        break;
    case UMUGU_WAVE_SQUARE:
        um_oscillator_square(&self->osc, &node->out_pipe);
        break;
    case UMUGU_WAVE_WHITE_NOISE:
        um_noisegen_white(&self->noise, &node->out_pipe);
        break;
    }

    node->iteration = umugu_get_context()->pipeline_iteration;
    return UMUGU_SUCCESS;
}

umugu_node_fn
um_oscil_getfn(umugu_fn fn)
{
    switch (fn) {
    case UMUGU_FN_INIT:
        return um_oscil_init;
    case UMUGU_FN_DEFAULTS:
        return um_oscil_defaults;
    case UMUGU_FN_PROCESS:
        return um_oscil_process;
    default:
        return NULL;
    }
}
