#include "umugu.h"
#include "umugu_internal.h"

#include <assert.h>

static inline int
um_amplitude_init(umugu_node *node)
{
    node->out_pipe.samples = NULL;
    node->out_pipe.count = 0;
    return UMUGU_SUCCESS;
}

static inline int
um_amplitude_defaults(umugu_node *node)
{
    um_amplitude *self = (void *)node;
    self->multiplier = 1.0f;
    return UMUGU_SUCCESS;
}

static inline int
um_amplitude_process(umugu_node *node)
{
    um_node_check_iteration(node);
    um_amplitude *self = (void *)node;
    const umugu_node *input = um_node_get_input(node);
    node->out_pipe.channels = input->out_pipe.channels;
    umugu_sample *out = umugu_alloc_signal(&node->out_pipe);

    const int size = node->out_pipe.count * node->out_pipe.channels;
    assert(size > 0);
    for (int i = 0; i < size; ++i) {
        out[i] = input->out_pipe.samples[i] * self->multiplier;
    }

    node->iteration = umugu_get_context()->pipeline_iteration;
    return UMUGU_SUCCESS;
}

umugu_node_fn
um_amplitude_getfn(umugu_fn fn)
{
    switch (fn) {
    case UMUGU_FN_INIT:
        return um_amplitude_init;
    case UMUGU_FN_DEFAULTS:
        return um_amplitude_defaults;
    case UMUGU_FN_PROCESS:
        return um_amplitude_process;
    default:
        return NULL;
    }
}
