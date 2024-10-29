#include "builtin_nodes.h"
#include "umugu.h"

#include <string.h>

static inline int um__init(umugu_node *node) {
    node->out_pipe_ready = 0;
    node->out_pipe_type = UMUGU_PIPE_SIGNAL;
    return UMUGU_SUCCESS;
}

static inline int um__defaults(umugu_node *node) {
    um__oscope *self = (void *)node;
    self->waveform = UMUGU_WAVEFORM_SINE;
    self->freq = 440;
    return UMUGU_SUCCESS;
}

static inline int um__process(umugu_node *node) {
    if (node->out_pipe_ready) {
        return UMUGU_NOOP;
    }

    um__oscope *self = (void *)node;
    umugu_frame *out = umugu_get_temp_signal(&node->out_pipe);

    if (!self->freq) {
        memset(out, 0, node->out_pipe.count * sizeof(umugu_frame));
        return UMUGU_SUCCESS;
    }

    for (int i = 0; i < node->out_pipe.count; i++) {
        float sample = umugu_waveform_lut[self->waveform][self->phase];
        out[i].left = sample;
        out[i].right = sample;
        self->phase += self->freq;
        if (self->phase >= UMUGU_SAMPLE_RATE) {
            self->phase -= UMUGU_SAMPLE_RATE;
        }
    }
    node->out_pipe_ready = 1;
    return UMUGU_SUCCESS;
}

umugu_node_fn um__oscope_getfn(umugu_fn fn) {
    switch (fn) {
    case UMUGU_FN_INIT:
        return um__init;
    case UMUGU_FN_DEFAULTS:
        return um__defaults;
    case UMUGU_FN_PROCESS:
        return um__process;
    default:
        return NULL;
    }
}
