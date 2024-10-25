#include "builtin_nodes.h"
#include "umugu.h"

#include <assert.h>

typedef struct {
    umugu_node node;
} um__output;

const int32_t um__output_size = (int32_t)sizeof(um__output);
const int32_t um__output_var_count = 0;
const umugu_var_info um__output_vars[1];

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
    umugu_node *input = ctx->pipeline.nodes[node->pipe_in_node_idx];
    if (!input->pipe_out_ready) {
        umugu_node_dispatch(input, UMUGU_FN_PROCESS);
        assert(input->pipe_out_ready);
    }

    node->pipe_out = ctx->io.out_audio_signal;
    for (int i = 0; i < node->pipe_out.count; ++i) {
        node->pipe_out.frames[i].left = input->pipe_out.frames[i].left;
        node->pipe_out.frames[i].right = input->pipe_out.frames[i].right;
    }

    /* TODO: SampleRate and type conversions if the output
        signal is different. */

    node->pipe_out_ready = 1;
    return UMUGU_SUCCESS;
}

umugu_node_fn um__output_getfn(umugu_fn fn) {
    switch (fn) {
    case UMUGU_FN_INIT:
        return um__init;
    case UMUGU_FN_PROCESS:
        return um__process;
    default:
        return NULL;
    }
}
