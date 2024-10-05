#include "builtin_nodes.h"
#include "umugu.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    umugu_node node;
    umugu_sample *audio_source;
    size_t sample_capacity;
    char filename[UMUGU_PATH_LEN];
    void *file_handle;
} um__wavplayer;

const int32_t um__wavplayer_size = (int32_t)sizeof(um__wavplayer);
const int32_t um__wavplayer_var_count = 1;

const umugu_var_info um__wavplayer_vars[] = {
    {
        .name = {.str = "Filename"},
        .offset_bytes = offsetof(um__wavplayer, filename),
        .type = UMUGU_TYPE_TEXT,
        .count = UMUGU_PATH_LEN,
        .range_min = 0.0f,
        .range_max = 127.0f
    }
};


static inline int um__init(umugu_node **node, umugu_signal *_)
{
    (void)_;
    umugu_ctx *ctx = umugu_get_context();
    um__wavplayer *self = (void*)*node;
    *node = (void*)((char*)*node + sizeof(um__wavplayer));

    self->file_handle = fopen(self->filename, "rb");
    if (!self->file_handle) {
        ctx->log("Couldn't open %s\n", self->filename); 
        /* TODO(qol): dummy wav file as a backup */
        ctx->abort();
        return UMUGU_ERR_FILE;
    }

    self->sample_capacity = ctx->audio_src_sample_capacity;
    self->audio_source = ctx->alloc(self->sample_capacity * sizeof(umugu_sample));
    return UMUGU_SUCCESS;
}

static inline int um__getsignal(umugu_node **node, umugu_signal *out)
{
    umugu_ctx *ctx = umugu_get_context();
    const float inv_sig_range = 1.0f / 32768.0f;
    um__wavplayer *self = (void*)*node;
    *node = (void*)((char*)*node + sizeof(um__wavplayer));
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
            ctx->log("Error (wav_player): BadAlloc.\n");
            self->sample_capacity = 0;
            self->audio_source = NULL;
            ctx->abort();
            return UMUGU_ERR_MEM;
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
    return UMUGU_SUCCESS;
}

umugu_node_fn um__wavplayer_getfn(umugu_fn fn)
{
    switch (fn)
    {
        case UMUGU_FN_INIT: return um__init;
        case UMUGU_FN_GETSIGNAL: return um__getsignal;
        default: return NULL;
    }
}
