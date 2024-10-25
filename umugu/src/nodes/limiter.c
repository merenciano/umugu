#include "builtin_nodes.h"

#include <assert.h>
#include <math.h>

typedef struct {
    umugu_node node;
    float min;
    float max;
} um__limiter;

const int32_t um__limiter_size = (int32_t)sizeof(um__limiter);
const int32_t um__limiter_var_count = 2;

const umugu_var_info um__limiter_vars[] = {
    {.name = {.str = "Min"},
     .offset_bytes = offsetof(um__limiter, min),
     .type = UMUGU_TYPE_FLOAT,
     .count = 1,
     .misc.range.min = -5.0f,
     .misc.range.max = 5.0f},
    {.name = {.str = "Max"},
     .offset_bytes = offsetof(um__limiter, max),
     .type = UMUGU_TYPE_FLOAT,
     .count = 1,
     .misc.range.min = -5.0f,
     .misc.range.max = 5.0f}};

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
    um__limiter *self = (void *)node;

    umugu_node *input = ctx->pipeline.nodes[node->pipe_in_node_idx];
    if (!input->pipe_out_ready) {
        umugu_node_dispatch(input, UMUGU_FN_PROCESS);
        assert(input->pipe_out_ready);
    }

    umugu_frame *out = umugu_get_temp_signal(&node->pipe_out);

    for (int i = 0; i < node->pipe_out.count; ++i) {
        out[i].left = fmin(input->pipe_out.frames[i].left, self->max);
        out[i].left = fmax(input->pipe_out.frames[i].left, self->min);
        out[i].right = fmin(input->pipe_out.frames[i].right, self->max);
        out[i].right = fmax(input->pipe_out.frames[i].right, self->min);
    }

    node->pipe_out_ready = 1;
    return UMUGU_SUCCESS;
}

umugu_node_fn um__limiter_getfn(umugu_fn fn) {
    switch (fn) {
    case UMUGU_FN_INIT:
        return um__init;
    case UMUGU_FN_PROCESS:
        return um__process;
    default:
        return NULL;
    }
}
