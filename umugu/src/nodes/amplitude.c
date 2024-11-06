#include "builtin_nodes.h"
#include "umugu.h"

#include <assert.h>

static inline int um__init(umugu_node *node) {
    node->out_pipe.samples = NULL;
    node->out_pipe.count = 0;
    return UMUGU_SUCCESS;
}

static inline int um__defaults(umugu_node *node) {
    um__amplitude *self = (void *)node;
    self->multiplier = 1.0f;
    return UMUGU_SUCCESS;
}

static inline int um__process(umugu_node *node) {
    if (node->out_pipe.samples) {
        return UMUGU_NOOP;
    }

    umugu_ctx *ctx = umugu_get_context();
    um__amplitude *self = (void *)node;

    umugu_node *input = ctx->pipeline.nodes[node->in_pipe_node];
    if (!input->out_pipe.samples) {
        umugu_node_dispatch(input, UMUGU_FN_PROCESS);
        assert(input->out_pipe.samples);
    }

    node->out_pipe.channels = input->out_pipe.channels;
    umugu_sample *out = umugu_alloc_signal(&node->out_pipe);

    const int size = node->out_pipe.count * node->out_pipe.channels;
    assert(size > 0);
    for (int i = 0; i < size; ++i) {
        out[i] = input->out_pipe.samples[i] * self->multiplier;
    }

    return UMUGU_SUCCESS;
}

umugu_node_fn um__amplitude_getfn(umugu_fn fn) {
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
