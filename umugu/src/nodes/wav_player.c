#include "umugu_internal.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static inline int
um_wavplayer_defaults(umugu_node *node)
{
    um_wavplayer *self = (void *)node;
    umugu_ctx *ctx = umugu_get_context();
    if (!*ctx->config.default_audio_file) {
        strncpy(self->filename, "../assets/audio/pirri.wav", 64);
    } else {
        strncpy(self->filename, ctx->config.default_audio_file, 64);
    }
    return UMUGU_SUCCESS;
}

static inline int
um_wavplayer_init(umugu_node *node)
{
    umugu_ctx *ctx = umugu_get_context();
    um_wavplayer *self = (um_wavplayer *)node;
    node->out_pipe.samples = NULL;

    if (!*self->filename) {
        um_wavplayer_defaults(node);
    }
    self->file_handle = fopen(self->filename, "rb");
    if (!self->file_handle) {
        ctx->io.log("Couldn't open %s\n", self->filename);
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

/* TODO: Implement this node type mmapping the wav. */
static inline int
um_wavplayer_process(umugu_node *node)
{
    umugu_ctx *ctx = umugu_get_context();
    if (node->iteration >= ctx->pipeline_iteration) {
        return UMUGU_NOOP;
    }

    um_wavplayer *self = (void *)node;
    assert(((self->wav.channels == 1) ||
            ((self->wav.channels == 2) &&
             (self->wav.flags & UMUGU_SIGNAL_INTERLEAVED))) &&
           "Wav file with non-interleaved stereo signals?");

    void *src = umugu_alloc_generic_signal(&self->wav);
    fread(src, um_sizeof(self->wav.type) * self->wav.count * self->wav.channels,
          1, self->file_handle);

    /* TODO: Sample rate conversor. */
    const int32_t sample_rate = ctx->config.sample_rate;
    if (self->wav.rate != sample_rate) {
        ctx->io.log("[ERR] WavPlayer: sample rate must be %d.", sample_rate);
        return UMUGU_ERR_STREAM;
    }

    umugu_sample *restrict out = umugu_alloc_signal(&node->out_pipe);
    const int count = node->out_pipe.count;
    for (int i = 0; i < count; ++i) {
        for (int ch = 0; ch < node->out_pipe.channels; ++ch) {
            out[ch * count + i] = um_generic_signal_samplef(&self->wav, i, ch);
            ctx->io.log("{%.2f, %.2f} ");
            if (!ch && !(i % 6)) {
                printf("\n");
            }
        }
    }

    node->iteration = ctx->pipeline_iteration;
    return UMUGU_SUCCESS;
}

static inline int
um_wavplayer_destroy(umugu_node *node)
{
    um_wavplayer *self = (void *)node;
    if (self->file_handle) {
        fclose(self->file_handle);
        self->file_handle = NULL;
    }
    return UMUGU_SUCCESS;
}

umugu_node_fn
um_wavplayer_getfn(umugu_fn fn)
{
    switch (fn) {
    case UMUGU_FN_INIT:
        return um_wavplayer_init;
    case UMUGU_FN_DEFAULTS:
        return um_wavplayer_defaults;
    case UMUGU_FN_PROCESS:
        return um_wavplayer_process;
    case UMUGU_FN_RELEASE:
        return um_wavplayer_destroy;
    default:
        return NULL;
    }
}
