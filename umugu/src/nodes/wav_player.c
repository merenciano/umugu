#include "builtin_nodes.h"
#include "umugu.h"
#include "umutils.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static inline umugu_sample um__sample(umugu_generic_signal *s, int i, int ch) {
    void *sample = umu_generic_signal_sample(s, i, ch);
    switch (s->type) {
    case UMUGU_TYPE_UINT8:
        return ((umugu_sample)(*(uint8_t *)sample) / 255.0f) * 2.0f - 1.0f;
    case UMUGU_TYPE_INT16:
        return (umugu_sample)(*(int16_t *)sample) / 32768.0f;
    case UMUGU_TYPE_FLOAT:
        return (umugu_sample)(*(float *)sample);
    default:
        assert(0);
        return 0.0f;
    }
}

static inline int um__init(umugu_node *node) {
    umugu_ctx *ctx = umugu_get_context();
    um__wavplayer *self = (um__wavplayer *)node;
    node->out_pipe.samples = NULL;

    self->file_handle = fopen(self->filename, "rb");
    if (!self->file_handle) {
        ctx->io.log("Couldn't open %s\n", self->filename);
        /* TODO(qol): dummy wav file as a backup */
        return UMUGU_ERR_FILE;
    }

    /* Advance the file read ptr to where the waveform data starts.
     * While I'm here I'm going to assign the signal format from the header. */
    char header[44];
    fread(header, 44, 1, self->file_handle);
    self->wav.flags |= UMUGU_SIGNAL_INTERLEAVED;
    self->wav.count = 0;
    self->wav.channels = *(int16_t *)(header + 22);
    self->wav.rate = *(int *)(header + 24);

    const int sample_size = header[34] / 8;
    if (sample_size == 2) {
        self->wav.type = UMUGU_TYPE_INT16;
    } else if (sample_size == 4) {
        self->wav.type = UMUGU_TYPE_FLOAT;
    } else if (sample_size == 1) {
        self->wav.type = UMUGU_TYPE_UINT8;
    } else {
        assert(0 && "Unexpected sample_size.");
    }

    /* The standard way would be checking first if it's int or float..
     * int type = header[20] == 1 ? INTEGER : FLOAT; // It can be 1
     * (int) or 3 (float) And then checking the size in bytes..
     * sample_bytes = header[34] / 8; Note that in WAV format, integers
     * of 8- bits are unsigned and 9+ signed.
     */

    node->out_pipe.channels = self->wav.channels;
    return UMUGU_SUCCESS;
}

static inline int um__defaults(umugu_node *node) {
    um__wavplayer *self = (void *)node;
    self->node.in_pipe_node = UMUGU_BADIDX;
    strncpy(self->filename, "../assets/audio/pirri.wav", 64);
    return UMUGU_SUCCESS;
}

/* TODO: Implement this node type mmapping the wav. */
static inline int um__process(umugu_node *node) {
    if (node->out_pipe.samples) {
        return UMUGU_NOOP;
    }

    um__wavplayer *self = (void *)node;
    void *src = umugu_get_temp_generic_signal(&self->wav);
    fread(src,
          umu_type_size(self->wav.type) * self->wav.count * self->wav.channels,
          1, self->file_handle);

    /* TODO: Sample rate conversor. */
    if (self->wav.rate != UMUGU_SAMPLE_RATE) {
        umugu_get_context()->io.log("[ERR] WavPlayer: sample rate must be %d.",
                                    UMUGU_SAMPLE_RATE);
        return UMUGU_ERR_STREAM;
    }

    umugu_sample *out = umugu_alloc_signal_buffer(&node->out_pipe);

    const int count = node->out_pipe.count;
    for (int ch = 0; ch < node->out_pipe.channels; ++ch) {
        for (int i = 0; i < count; ++i) {
            out[ch * count + i] = um__sample(&self->wav, i, ch);
        }
    }

    return UMUGU_SUCCESS;
}

static int um__destroy(umugu_node *node) {
    um__wavplayer *self = (void *)node;
    if (self->file_handle) {
        fclose(self->file_handle);
        self->file_handle = NULL;
    }
    return UMUGU_SUCCESS;
}

umugu_node_fn um__wavplayer_getfn(umugu_fn fn) {
    switch (fn) {
    case UMUGU_FN_INIT:
        return um__init;
    case UMUGU_FN_DEFAULTS:
        return um__defaults;
    case UMUGU_FN_PROCESS:
        return um__process;
    case UMUGU_FN_RELEASE:
        return um__destroy;
    default:
        return NULL;
    }
}
