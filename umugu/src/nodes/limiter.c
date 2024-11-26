#include "umugu.h"
#include "umugu_internal.h"

#include <math.h>

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
        out[i] = fmin(input->out_pipe.samples[i], self->max);
        out[i] = fmax(input->out_pipe.samples[i], self->min);
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
