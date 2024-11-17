#include "umugu.h"
#include "umugu_internal.h"

#include <assert.h>
#include <math.h>

static inline int
um_limiter_init(umugu_node *node)
{
    node->out_pipe.samples = NULL;
    node->out_pipe.count = 0;
    return UMUGU_SUCCESS;
}

static inline int
um_limiter_defaults(umugu_node *node)
{
    um_limiter *self = (void *)node;
    self->min = -1.0f;
    self->max = 1.0f;
    return UMUGU_SUCCESS;
}

static inline int
um_limiter_process(umugu_node *node)
{
    um_node_check_iteration(node);
    um_limiter *self = (void *)node;

    const umugu_node *input = um_node_get_input(node);
    node->out_pipe.channels = input->out_pipe.channels;
    umugu_sample *out = umugu_alloc_signal(&node->out_pipe);

    for (int i = 0; i < node->out_pipe.count * node->out_pipe.channels; ++i) {
        out[i] = fmin(input->out_pipe.samples[i], self->max);
        out[i] = fmax(input->out_pipe.samples[i], self->min);
    }

    node->iteration = umugu_get_context()->pipeline_iteration;
    return UMUGU_SUCCESS;
}

umugu_node_fn
um_limiter_getfn(umugu_fn fn)
{
    switch (fn) {
    case UMUGU_FN_INIT:
        return um_limiter_init;
    case UMUGU_FN_DEFAULTS:
        return um_limiter_defaults;
    case UMUGU_FN_PROCESS:
        return um_limiter_process;
    default:
        return NULL;
    }
}
