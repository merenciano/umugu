#include "builtin_nodes.h"
#include "umugu.h"

#include <assert.h>

static inline int um__init(umugu_node *node) {
    node->pipe_out_type = UMUGU_PIPE_SIGNAL;
    node->pipe_out_ready = 0;
    return UMUGU_SUCCESS;
}

static inline int um__process(umugu_node *node) {
    if (node->pipe_out_ready) {
        return UMUGU_NOOP;
    }

    umugu_ctx *ctx = umugu_get_context();
    um__amplitude *self = (void *)node;

    umugu_node *input = ctx->pipeline.nodes[node->pipe_in_node_idx];
    if (!input->pipe_out_ready) {
        umugu_node_dispatch(input, UMUGU_FN_PROCESS);
        assert(input->pipe_out_ready);
    }

    umugu_frame *out = umugu_get_temp_signal(&node->pipe_out);

    for (int i = 0; i < node->pipe_out.count; ++i) {
        out[i].left = input->pipe_out.frames[i].left * self->multiplier;
        out[i].right = input->pipe_out.frames[i].right * self->multiplier;
    }

    node->pipe_out_ready = 1;

    return UMUGU_SUCCESS;
}

umugu_node_fn um__amplitude_getfn(umugu_fn fn) {
    switch (fn) {
    case UMUGU_FN_INIT:
        return um__init;
    case UMUGU_FN_PROCESS:
        return um__process;
    default:
        return NULL;
    }
}
