#include "builtin_nodes.h"
#include "umugu.h"

#include <assert.h>

static inline int um__init(umugu_node *node) {
    node->out_pipe_type = UMUGU_PIPE_SIGNAL;
    node->out_pipe_ready = 0;
    return UMUGU_SUCCESS;
}

static inline int um__process(umugu_node *node) {
    if (node->out_pipe_ready) {
        return UMUGU_NOOP;
    }

    umugu_ctx *ctx = umugu_get_context();
    umugu_node *input = ctx->pipeline.nodes[node->in_pipe_node];
    if (!input->out_pipe_ready) {
        umugu_node_dispatch(input, UMUGU_FN_PROCESS);
        assert(input->out_pipe_ready);
    }

    node->out_pipe = ctx->io.out_audio_signal;
    for (int i = 0; i < node->out_pipe.count; ++i) {
        node->out_pipe.frames[i].left = input->out_pipe.frames[i].left;
        node->out_pipe.frames[i].right = input->out_pipe.frames[i].right;
    }

    /* TODO: SampleRate and type conversions if the output
        signal is different. */

    node->out_pipe_ready = 1;
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
