#include "builtin_nodes.h"

#include <assert.h>
#include <math.h>

static inline int um__init(umugu_node *node) {
    node->out_pipe.samples = NULL;
    node->out_pipe.count = 0;
    return UMUGU_SUCCESS;
}

static inline int um__defaults(umugu_node *node) {
    um__limiter *self = (void *)node;
    self->min = -1.0f;
    self->max = 1.0f;
    return UMUGU_SUCCESS;
}

static inline int um__process(umugu_node *node) {
    if (node->out_pipe.samples) {
        return UMUGU_NOOP;
    }

    umugu_ctx *ctx = umugu_get_context();
    um__limiter *self = (void *)node;

    umugu_node *input = ctx->pipeline.nodes[node->in_pipe_node];
    if (!input->out_pipe.samples) {
        umugu_node_dispatch(input, UMUGU_FN_PROCESS);
        assert(input->out_pipe.samples);
    }

    node->out_pipe.channels = input->out_pipe.channels;
    umugu_sample *out = umugu_alloc_signal_buffer(&node->out_pipe);

    for (int i = 0; i < node->out_pipe.count * node->out_pipe.channels; ++i) {
        out[i] = fmin(input->out_pipe.samples[i], self->max);
        out[i] = fmax(input->out_pipe.samples[i], self->min);
    }

    return UMUGU_SUCCESS;
}

umugu_node_fn um__limiter_getfn(umugu_fn fn) {
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
