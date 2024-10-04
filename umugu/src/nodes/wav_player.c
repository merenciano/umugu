#include "umugu_internal.h"
#include <stdio.h>
#include <string.h>

static inline void um__init(umugu_node **node, umugu_signal *_)
{
    um__unused(_);
    umugu_ctx *ctx = umugu_get_context();

    um__wav_player *self = (void*)*node;
    *node = (void*)((char*)*node + sizeof(um__wav_player));

    self->file_handle = fopen(self->filename, "rb");
    if (!self->file_handle) {
        ctx->err = UMUGU_ERR_FILE;
        snprintf(ctx->err_msg, UMUGU_ERR_MSG_LEN, "Wav player - Can't open file %s", self->filename);
        ctx->log(UMUGU_LOG_ERR, "Couldn't open %s\n", self->filename); 
        /* TODO(qol): dummy wav file as a backup */
        ctx->abort();
    }

    self->sample_capacity = ctx->audio_src_sample_capacity;
    self->audio_source = ctx->alloc(self->sample_capacity * sizeof(umugu_sample));
}

static inline void um__getsignal(umugu_node **node, umugu_signal *out)
{
    umugu_ctx *ctx = umugu_get_context();
    const float inv_sig_range = 1.0f / 32768.0f;
    um__wav_player *self = (void*)*node;
    *node = (void*)((char*)*node + sizeof(um__wav_player));
    const int64_t count = out->count;

    if (count > (int64_t)self->sample_capacity) {
        self->sample_capacity *= 2;
        if (self->sample_capacity < 256) {
            self->sample_capacity = 256;
        }

        ctx->free(self->audio_source);
        /* No point on checking the alloc return in a temp variable since
           keeping a buffer smaller than count is critical error anyway.
           TODO(qol): add a static zeroed audio source as a backup in order
                      to not abort if one source fails.
         */
        self->audio_source = ctx->alloc(self->sample_capacity * sizeof(umugu_sample));
        if (!self->audio_source) {
            ctx->err = UMUGU_ERR_MEM;
            strncpy(ctx->err_msg, "Error: BadAlloc. Ignoring Wav player this frame.", UMUGU_ERR_MSG_LEN);
            self->sample_capacity = 0;
            self->audio_source = NULL;
            ctx->abort();
            return;
        }
    }

    /* TODO: Support Mono and different sample rate than output stream. */
    fread(self->audio_source, count * 4, 1, (FILE*)self->file_handle); /* size 4 -> 2 bytes stereo */

    short* it = (short*)self->audio_source + count * 2 - 1;
    for (int i = count - 1; i >= 0; --i) {
        self->audio_source[i].right = *it-- * inv_sig_range;
        self->audio_source[i].left = *it-- * inv_sig_range;
    }

    out->samples = self->audio_source;
}

umugu_node_fn um__wavplayer_getfn(umugu_code fn)
{
    switch (fn)
    {
        case UMUGU_FN_INIT: return um__init;
        case UMUGU_FN_GETSIGNAL: return um__getsignal;
        default: return NULL;
    }
}
