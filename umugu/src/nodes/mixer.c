#include "builtin_nodes.h"
#include "umugu.h"
#include "umutils.h"

#include <assert.h>
#include <string.h>

#define EMPTY_COUNT 1024
static const umugu_sample EMPTY_SAMPLES[EMPTY_COUNT] = {0};

static inline int um__init(umugu_node *node) {
    node->out_pipe.samples = NULL;
    node->out_pipe.channels = 1;
    return UMUGU_SUCCESS;
}

static inline int um__defaults(umugu_node *node) {
    um__mixer *self = (void *)node;
    self->extra_pipe_in_node_idx[0] = 0;
    self->input_count = 2;
    return UMUGU_SUCCESS;
}

static inline int um__process(umugu_node *node) {
    if (node->out_pipe.samples) {
        return UMUGU_NOOP;
    }

    umugu_ctx *ctx = umugu_get_context();
    um__mixer *self = (void *)node;

    umugu_sample *out = umugu_alloc_signal_buffer(&node->out_pipe);
    const int sample_count = node->out_pipe.count;
    int signals = 0;

    umugu_node *input = ctx->pipeline.nodes[node->in_pipe_node];
    if (!input->out_pipe.samples) {
        umugu_node_dispatch(input, UMUGU_FN_PROCESS);
        assert(input->out_pipe.samples);
    }

    umugu_sample *in_samples =
        umu_signal_get_channel(input->out_pipe, node->in_pipe_channel);

    ++signals;
    memcpy(out, in_samples, sizeof(umugu_sample) * sample_count);

    for (int i = 0; i < self->input_count - 1; ++i) {
        umugu_node *in = ctx->pipeline.nodes[self->extra_pipe_in_node_idx[i]];
        if (!in->out_pipe.samples) {
            umugu_node_dispatch(node, UMUGU_FN_PROCESS);
            assert(in->out_pipe.samples);
        }

        umugu_sample *in_samples = umu_signal_get_channel(
            in->out_pipe, self->extra_pipe_in_channel[i]);
        if (memcmp(in_samples, EMPTY_SAMPLES,
                   sizeof(umugu_sample) * (in->out_pipe.count < EMPTY_COUNT
                                               ? in->out_pipe.count
                                               : EMPTY_COUNT))) {
            ++signals;
        }

        for (int j = 0; j < sample_count; ++j) {
            out[j] += in_samples[j];
        }
    }

    /* Normalize */
    const float inv_count = 1.0f / signals;
    for (int i = 0; i < sample_count; i++) {
        out[i] *= inv_count;
    }

    return UMUGU_SUCCESS;
}

umugu_node_fn um__mixer_getfn(umugu_fn fn) {
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
