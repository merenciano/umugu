#include "builtin_nodes.h"
#include "umugu.h"

#include <assert.h>
#include <math.h>
#include <string.h>

static const float notes_hz[12] = {261.63f, 277.18f, 293.66f, 311.13f,
                                   329.63f, 349.23f, 369.99f, 392.0f,
                                   415.3f,  440.0f,  466.16f, 493.88f};

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
    um__piano *self = (void *)node;

    umugu_node *input = ctx->pipeline.nodes[node->pipe_in_node_idx];
    if (!input->pipe_out_ready) {
        umugu_node_dispatch(input, UMUGU_FN_PROCESS);
        assert(input->pipe_out_ready);
    }

    umugu_frame *out = umugu_get_temp_signal(&node->pipe_out);
    memset(out, 0, sizeof(umugu_frame) * node->pipe_out.count);
    umugu_ctrl *ctrl = &ctx->pipeline.control;

    int notes = 0;
    for (int note = 0; note < UMUGU_NOTE_COUNT; ++note) {
        if (ctrl->notes[note] > 0) {
            int oct = note / 12;
            int n = note % 12;
            float freq = notes_hz[n] * powf(2.0f, oct - 4.0f);
            for (int i = 0; i < node->pipe_out.count; ++i) {
                float sample =
                    umugu_waveform_lut[self->waveform][self->phase[note]];
                out[i].left += sample;
                out[i].right += sample;
                self->phase[note] += freq;
                if (self->phase[note] >= UMUGU_SAMPLE_RATE) {
                    self->phase[note] -= UMUGU_SAMPLE_RATE;
                }
            }
            ++notes;
        }
    }

    float inv = 1.0f / notes;
    for (int i = 0; i < node->pipe_out.count; ++i) {
        out[i].left *= inv;
        out[i].right *= inv;
    }
    node->pipe_out_ready = 1;

    return UMUGU_SUCCESS;
}

umugu_node_fn um__piano_getfn(umugu_fn fn) {
    switch (fn) {
    case UMUGU_FN_INIT:
        return um__init;
    case UMUGU_FN_PROCESS:
        return um__process;
    default:
        return NULL;
    }
}
