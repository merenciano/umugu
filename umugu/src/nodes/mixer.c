#include "builtin_nodes.h"
#include "umugu.h"

#include <assert.h>
#include <string.h>

#define UMUGU_MIXER_MAX_INPUTS 8

typedef struct {
    umugu_node node;
    int32_t input_count;
    int16_t extra_pipe_in_node_idx[UMUGU_MIXER_MAX_INPUTS];
    int8_t extra_pipe_in_channel[UMUGU_MIXER_MAX_INPUTS];
} um__mixer;

const int32_t um__mixer_size = (int32_t)sizeof(um__mixer);
const int32_t um__mixer_var_count = 1;

const umugu_var_info um__mixer_vars[] = {
    {.name = {.str = "InputCount"},
     .offset_bytes = offsetof(um__mixer, input_count),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .misc.range.min = 0,
     .misc.range.max = 32},
    {.name = {.str = "ExtraInputPipeNodeIdx"},
     .offset_bytes = offsetof(um__mixer, extra_pipe_in_node_idx),
     .type = UMUGU_TYPE_INT16,
     .count = UMUGU_MIXER_MAX_INPUTS,
     .misc.range.min = 0,
     .misc.range.max = 9999},
    {.name = {.str = "ExtraInputPipeChannel"},
     .offset_bytes = offsetof(um__mixer, extra_pipe_in_channel),
     .type = UMUGU_TYPE_INT8,
     .count = UMUGU_MIXER_MAX_INPUTS,
     .misc.range.min = 0,
     .misc.range.max = UMUGU_CHANNELS}};

static inline int um__init(umugu_node *node) {
    node->pipe_out_ready = 0;
    node->pipe_out_type = UMUGU_PIPE_SIGNAL;
    return UMUGU_SUCCESS;
}

static umugu_frame frames_empty[256];

static inline int um__process(umugu_node *node) {
    if (node->pipe_out_ready) {
        return UMUGU_NOOP;
    }

    umugu_ctx *ctx = umugu_get_context();
    um__mixer *self = (void *)node;

    umugu_frame *out = umugu_get_temp_signal(&node->pipe_out);
    const int sample_count = node->pipe_out.count;
    int signals = 0;

    umugu_node *input = ctx->pipeline.nodes[node->pipe_in_node_idx];
    if (!input->pipe_out_ready) {
        umugu_node_dispatch(input, UMUGU_FN_PROCESS);
        assert(input->pipe_out_ready);
    }

    ++signals;
    memcpy(out, input->pipe_out.frames,
           sizeof(umugu_frame) * node->pipe_out.count);

    for (int i = 0; i < self->input_count - 1; ++i) {
        umugu_node *in = ctx->pipeline.nodes[self->extra_pipe_in_node_idx[i]];
        if (!in->pipe_out_ready) {
            umugu_node_dispatch(node, UMUGU_FN_PROCESS);
            assert(in->pipe_out_ready);
        }

        if (memcmp(in->pipe_out.frames, frames_empty,
                   in->pipe_out.count < 256 ? in->pipe_out.count : 256)) {
            ++signals;
        }

        for (int j = 0; j < sample_count; ++j) {
            out[j].left += in->pipe_out.frames[j].left;
            out[j].right += in->pipe_out.frames[j].right;
        }
    }

    float inv_count = 1.0f / signals;
    for (int i = 0; i < sample_count; i++) {
        out[i].left *= inv_count;
        out[i].right *= inv_count;
    }

    node->pipe_out_ready = 1;

    return UMUGU_SUCCESS;
}

umugu_node_fn um__mixer_getfn(umugu_fn fn) {
    switch (fn) {
    case UMUGU_FN_INIT:
        return um__init;
    case UMUGU_FN_PROCESS:
        return um__process;
    default:
        return NULL;
    }
}
