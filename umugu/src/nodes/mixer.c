#include "umugu.h"
#include "umugu_internal.h"

#include <assert.h>
#include <string.h>

#define EMPTY_COUNT 128
static const umugu_sample EMPTY_SAMPLES[EMPTY_COUNT] = {0};

static inline int
um_mixer_init(umugu_node *node)
{
    node->out_pipe.samples = NULL;
    node->out_pipe.count = 0;
    node->out_pipe.channels = 1;
    node->iteration = 0;
    return UMUGU_SUCCESS;
}

static inline int
um_mixer_defaults(umugu_node *node)
{
    um_mixer *self = (void *)node;
    self->extra_pipe_in_node_idx[0] = 0;
    self->input_count = 2;
    return UMUGU_SUCCESS;
}

static inline int
um_mixer_process(umugu_node *node)
{
    um_node_check_iteration(node);
    umugu_ctx *ctx = umugu_get_context();
    um_mixer *self = (void *)node;

    umugu_sample *out = umugu_alloc_signal(&node->out_pipe);
    const int sample_count = node->out_pipe.count;
    int signals = 0;

    const umugu_node *input = um_node_get_input(node);

    umugu_sample *in_samples =
        um_signal_get_channel(&input->out_pipe, node->input_channel);

    ++signals;
    memcpy(out, in_samples, sizeof(umugu_sample) * sample_count);

    for (int i = 0; i < self->input_count - 1; ++i) {
        umugu_node *in = ctx->pipeline.nodes[self->extra_pipe_in_node_idx[i]];
        if (!in->out_pipe.samples) {
            umugu_node_dispatch(node, UMUGU_FN_PROCESS);
            assert(in->out_pipe.samples);
        }

        umugu_sample *in_samples = um_signal_get_channel(
            &in->out_pipe, self->extra_pipe_in_channel[i]);
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

    node->iteration = ctx->pipeline_iteration;
    return UMUGU_SUCCESS;
}

umugu_node_fn
um_mixer_getfn(umugu_fn fn)
{
    switch (fn) {
    case UMUGU_FN_INIT:
        return um_mixer_init;
    case UMUGU_FN_DEFAULTS:
        return um_mixer_defaults;
    case UMUGU_FN_PROCESS:
        return um_mixer_process;
    default:
        return NULL;
    }
}
