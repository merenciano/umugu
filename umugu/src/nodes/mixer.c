#include "umugu.h"
#include "umugu_internal.h"

#include <string.h>

#define EMPTY_COUNT 128
static const float EMPTY_SAMPLES[EMPTY_COUNT] = {0};

static inline int
um_mixer_init(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    UM_UNUSED(ctx);
    if (flags & UMUGU_FN_INIT_DEFAULTS) {
        um_mixer *self = (void *)node;
        self->extra_pipe_in_node_idx[0] = 0;
        self->input_count = 2;
    }
    node->out_pipe.samples = NULL;
    node->out_pipe.frame_count = 0;
    node->out_pipe.channel_count = 1;
    return UMUGU_SUCCESS;
}

static inline int
um_mixer_process(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    um_mixer *self = (void *)node;
    float *out = um_alloc_samples(ctx, &node->out_pipe);
    const int sample_count = node->out_pipe.frame_count;
    int signals = 0;

    const umugu_node *input = um_node_get_input(ctx, node);

    float *in_samples = um_signal_get_channel(&input->out_pipe, node->input_channel);

    ++signals;
    memcpy(out, in_samples, sizeof(float) * sample_count);

    for (int i = 0; i < self->input_count - 1; ++i) {
        umugu_node *in = ctx->pipeline.nodes[self->extra_pipe_in_node_idx[i]];
        if (!in->out_pipe.samples) {
            umugu_node_dispatch(ctx, node, UMUGU_FN_PROCESS, flags);
            assert(in->out_pipe.samples);
        }

        float *in_samples = um_signal_get_channel(&in->out_pipe, self->extra_pipe_in_channel[i]);
        if (memcmp(
                in_samples, EMPTY_SAMPLES,
                sizeof(float) *
                    (in->out_pipe.frame_count < EMPTY_COUNT
                         ? in->out_pipe.frame_count
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

umugu_node_func
um_mixer_getfn(umugu_fn fn)
{
    switch (fn) {
    case UMUGU_FN_INIT:
        return um_mixer_init;
    case UMUGU_FN_PROCESS:
        return um_mixer_process;
    default:
        return NULL;
    }
}
