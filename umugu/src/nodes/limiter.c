#include "builtin_nodes.h"

#include <assert.h>
#include <math.h>

static inline int um__init(umugu_node *node) {
    node->out_pipe_type = UMUGU_PIPE_SIGNAL;
    node->out_pipe_ready = 0;
    return UMUGU_SUCCESS;
}

static inline int um__defaults(umugu_node *node) {
    um__limiter *self = (void *)node;
    self->min = -1.0f;
    self->max = 1.0f;
    return UMUGU_SUCCESS;
}

static inline int um__process(umugu_node *node) {
    if (node->out_pipe_ready) {
        return UMUGU_NOOP;
    }

    umugu_ctx *ctx = umugu_get_context();
    um__limiter *self = (void *)node;

    umugu_node *input = ctx->pipeline.nodes[node->in_pipe_node];
    if (!input->out_pipe_ready) {
        umugu_node_dispatch(input, UMUGU_FN_PROCESS);
        assert(input->out_pipe_ready);
    }

    umugu_frame *out = umugu_get_temp_signal(&node->out_pipe);

    for (int i = 0; i < node->out_pipe.count; ++i) {
        out[i].left = fmin(input->out_pipe.frames[i].left, self->max);
        out[i].left = fmax(input->out_pipe.frames[i].left, self->min);
        out[i].right = fmin(input->out_pipe.frames[i].right, self->max);
        out[i].right = fmax(input->out_pipe.frames[i].right, self->min);
    }

    node->out_pipe_ready = 1;
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
