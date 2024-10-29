#include "builtin_nodes.h"
#include "umugu.h"

#include <assert.h>
#include <string.h>

static umugu_frame frames_empty[256];

static inline int um__init(umugu_node *node) {
    node->out_pipe_ready = 0;
    node->out_pipe_type = UMUGU_PIPE_SIGNAL;
    return UMUGU_SUCCESS;
}

static inline int um__defaults(umugu_node *node) {
    um__mixer *self = (void *)node;
    self->extra_pipe_in_node_idx[0] = 0;
    self->input_count = 2;
    return UMUGU_SUCCESS;
}

static inline int um__process(umugu_node *node) {
    if (node->out_pipe_ready) {
        return UMUGU_NOOP;
    }

    umugu_ctx *ctx = umugu_get_context();
    um__mixer *self = (void *)node;

    umugu_frame *out = umugu_get_temp_signal(&node->out_pipe);
    const int sample_count = node->out_pipe.count;
    int signals = 0;

    umugu_node *input = ctx->pipeline.nodes[node->in_pipe_node];
    if (!input->out_pipe_ready) {
        umugu_node_dispatch(input, UMUGU_FN_PROCESS);
        assert(input->out_pipe_ready);
    }

    ++signals;
    memcpy(out, input->out_pipe.frames,
           sizeof(umugu_frame) * node->out_pipe.count);

    for (int i = 0; i < self->input_count - 1; ++i) {
        umugu_node *in = ctx->pipeline.nodes[self->extra_pipe_in_node_idx[i]];
        if (!in->out_pipe_ready) {
            umugu_node_dispatch(node, UMUGU_FN_PROCESS);
            assert(in->out_pipe_ready);
        }

        if (memcmp(in->out_pipe.frames, frames_empty,
                   in->out_pipe.count < 256 ? in->out_pipe.count : 256)) {
            ++signals;
        }

        for (int j = 0; j < sample_count; ++j) {
            out[j].left += in->out_pipe.frames[j].left;
            out[j].right += in->out_pipe.frames[j].right;
        }
    }

    float inv_count = 1.0f / signals;
    for (int i = 0; i < sample_count; i++) {
        out[i].left *= inv_count;
        out[i].right *= inv_count;
    }

    node->out_pipe_ready = 1;

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
