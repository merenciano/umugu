#include "umugu_internal.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static inline void
um_wavplayer_defaults(umugu_ctx *ctx, um_wavplayer *self)
{
    self->node.out_pipe.samples = NULL;
    if (!*ctx->fallback_wav_file) {
        strncpy(self->filename, "../assets/audio/pirri.wav", UMUGU_PATH_LEN); // src Pirri.wav
    } else {
        strncpy(self->filename, ctx->fallback_wav_file, UMUGU_PATH_LEN);
    }
}

static inline int
um_wavplayer_init(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    um_wavplayer *self = (um_wavplayer *)node;

    if (!*self->filename || (flags & UMUGU_FN_INIT_DEFAULTS)) {
        um_wavplayer_defaults(ctx, self);
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
    self->wav.interleaved_channels = true;
    self->wav.samples.frame_count = 0;
    self->wav.samples.channel_count = *(int16_t *)(header + 22);
    self->wav.sample_rate = *(int *)(header + 24);

    const int sample_size = header[34] / 8;
    if (sample_size == 2) {
        self->wav.format = UMUGU_TYPE_INT16;
    } else if (sample_size == 4) {
        self->wav.format = UMUGU_TYPE_FLOAT;
    } else if (sample_size == 1) {
        self->wav.format = UMUGU_TYPE_UINT8;
    } else {
        assert(0 && "Unexpected sample_size.");
    }

    node->out_pipe.channel_count = self->wav.samples.channel_count;
    return UMUGU_SUCCESS;
}

/* TODO: Implement this node type mmapping the wav. */
static inline int
um_wavplayer_process(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    UM_UNUSED(flags);
    um_wavplayer *self = (void *)node;
    assert(
        ((self->wav.samples.channel_count == 1) ||
         ((self->wav.samples.channel_count == 2) && (self->wav.interleaved_channels))) &&
        "Wav file with non-interleaved stereo signals?");

    void *src = um_alloc_signal(ctx, &self->wav);
    fread(
        src,
        um_type_sizeof(self->wav.format) * self->wav.samples.frame_count *
            self->wav.samples.channel_count,
        1, self->file_handle);

    /* TODO: Sample rate conversor. */
    const int sample_rate = ctx->pipeline.sig.sample_rate;
    if (self->wav.sample_rate != sample_rate) {
        ctx->io.log("[ERR] WavPlayer: sample rate must be %d.", sample_rate);
        return UMUGU_ERR_STREAM;
    }

    float *restrict out = um_alloc_samples(ctx, &node->out_pipe);
    const int count = node->out_pipe.frame_count;
    for (int i = 0; i < count; ++i) {
        for (int ch = 0; ch < node->out_pipe.channel_count; ++ch) {
            out[ch * count + i] = um_signal_samplef(&self->wav, i, ch);
            ctx->io.log("{%.2f, %.2f} ");
            if (!ch && !(i % 6)) {
                ctx->io.log("\n");
            }
        }
    }

    return UMUGU_SUCCESS;
}

static inline int
um_wavplayer_destroy(umugu_ctx *ctx, umugu_node *node, umugu_fn_flags flags)
{
    UM_UNUSED(ctx), UM_UNUSED(flags);
    um_wavplayer *self = (void *)node;
    if (self->file_handle) {
        fclose(self->file_handle);
        self->file_handle = NULL;
    }
    return UMUGU_SUCCESS;
}

umugu_node_func
um_wavplayer_getfn(umugu_fn fn)
{
    switch (fn) {
    case UMUGU_FN_INIT:
        return um_wavplayer_init;
    case UMUGU_FN_PROCESS:
        return um_wavplayer_process;
    case UMUGU_FN_RELEASE:
        return um_wavplayer_destroy;
    default:
        return NULL;
    }
}
