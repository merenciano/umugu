#include "umugu.h"
#include "umugu_internal.h"

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
