#include "builtin_nodes.h"
#include "umugu.h"
#include "umutils.h"

#include <assert.h>
#include <string.h>

static inline int um__init(umugu_node *node) {
    um__piano *self = (void *)node;
    node->out_pipe.samples = NULL;
    node->out_pipe.channels = 1;
    memset(self->phase, 0, sizeof(self->phase));
    return UMUGU_SUCCESS;
}

static inline int um__defaults(umugu_node *node) {
    um__piano *self = (void *)node;
    self->waveform = UMUGU_WAVEFORM_SINE;
    return UMUGU_SUCCESS;
}

static inline int um__process(umugu_node *node) {
    if (node->out_pipe.samples) {
        return UMUGU_NOOP;
    }

    umugu_ctx *ctx = umugu_get_context();
    um__piano *self = (void *)node;

    umugu_node *input = ctx->pipeline.nodes[node->in_pipe_node];
    if (!input->out_pipe.samples) {
        umugu_node_dispatch(input, UMUGU_FN_PROCESS);
        assert(input->out_pipe.samples);
    }

    umugu_sample *out = umugu_alloc_signal(&node->out_pipe);
    memset(out, 0, sizeof(umugu_sample) * node->out_pipe.count);
    umugu_ctrl *ctrl = &ctx->io.controller;

    int notes = 0;
    for (int note = 0; note < UMUGU_NOTE_COUNT; ++note) {
        if (ctrl->notes[note] > 0) {
            float freq = umu_note_freq(note);
            for (int i = 0; i < node->out_pipe.count; ++i) {
                out[i] +=
                    umugu_waveform_lut[self->waveform][(int)self->phase[note]];
                self->phase[note] += freq;
                if (self->phase[note] >= UMUGU_SAMPLE_RATE) {
                    self->phase[note] -= UMUGU_SAMPLE_RATE;
                }
            }
            ++notes;
        }
    }

    const float inv = 1.0f / notes;
    for (int i = 0; i < node->out_pipe.count; ++i) {
        out[i] *= inv;
    }

    return UMUGU_SUCCESS;
}

umugu_node_fn um__piano_getfn(umugu_fn fn) {
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
