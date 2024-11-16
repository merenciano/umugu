#include "umugu.h"
#include "umugu_internal.h"

#include <assert.h>
#include <string.h>

static inline int
um__init(umugu_node *node)
{
    um_piano *self = (void *)node;
    node->out_pipe.samples = NULL;
    node->out_pipe.channels = 1;
    memset(self->osc, 0, sizeof(self->osc));
    return UMUGU_SUCCESS;
}

static inline int
um__defaults(umugu_node *node)
{
    um_piano *self = (void *)node;
    self->waveform = UMUGU_WAVE_SINE;
    return UMUGU_SUCCESS;
}

static inline int
um__process(umugu_node *node)
{
    umugu_ctx *ctx = umugu_get_context();
    assert(node->iteration < ctx->pipeline_iteration);

    um_piano *self = (void *)node;
    umugu_node *input = ctx->pipeline.nodes[node->prev_node];
    assert(input->iteration > node->iteration);
#if 0
    if (!input->out_pipe.samples) {
        umugu_node_dispatch(input, UMUGU_FN_PROCESS);
        assert(input->out_pipe.samples);
    }
#endif

    umugu_sample *out = umugu_alloc_signal(&node->out_pipe);
    memset(out, 0, sizeof(umugu_sample) * node->out_pipe.count);
    umugu_ctrl *ctrl = &ctx->io.controller;

    const int count = node->out_pipe.count;
    int notes = 0;
    for (int note = 0; note < UMUGU_NOTE_COUNT; ++note) {
        if (ctrl->notes[note] > 0) {
            self->osc[note].freq = um_note_freq(note);
            umugu_signal tmp = node->out_pipe;
            umugu_alloc_signal(&tmp);
            um_oscillator_sine(&self->osc[note], &tmp);
            for (int i = 0; i < count; ++i) {
                node->out_pipe.samples[i] += tmp.samples[i];
            }
            ++notes;
        }
    }

    const float inv = 1.0f / notes;
    for (int i = 0; i < node->out_pipe.count; ++i) {
        out[i] *= inv;
    }

    node->iteration = ctx->pipeline_iteration;
    return UMUGU_SUCCESS;
}

umugu_node_fn
um_piano_getfn(umugu_fn fn)
{
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
