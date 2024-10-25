#include "builtin_nodes.h"
#include "umugu.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    umugu_node node;
    umugu_signal source;
    char filename[UMUGU_PATH_LEN];
    void *file_handle;
} um__wavplayer;

const int32_t um__wavplayer_size = (int32_t)sizeof(um__wavplayer);
const int32_t um__wavplayer_var_count = 4;

const umugu_var_info um__wavplayer_vars[] = {
    {.name = {.str = "Sample rate"},
     .offset_bytes =
         offsetof(um__wavplayer, source) + offsetof(umugu_signal, rate),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .misc.range.min = 0.0f,
     .misc.range.max = 96000.0f},
    {.name = {.str = "Value type"},
     .offset_bytes =
         offsetof(um__wavplayer, source) + offsetof(umugu_signal, type),
     .type = UMUGU_TYPE_INT16,
     .count = 1,
     .misc.range.min = 64,
     .misc.range.max = 128},
    {.name = {.str = "Channels"},
     .offset_bytes =
         offsetof(um__wavplayer, source) + offsetof(umugu_signal, channels),
     .type = UMUGU_TYPE_INT16,
     .count = 1,
     .misc.range.min = 1.0f,
     .misc.range.max = 16.0f},
    {.name = {.str = "Filename"},
     .offset_bytes = offsetof(um__wavplayer, filename),
     .type = UMUGU_TYPE_TEXT,
     .count = UMUGU_PATH_LEN,
     .misc.range.min = 0.0f,
     .misc.range.max = 127.0f}};

static inline int um__getsize(int type) {
    switch (type) {
    case UMUGU_TYPE_INT16:
        return 2;
    case UMUGU_TYPE_INT32:
        return 4;
    case UMUGU_TYPE_FLOAT:
        return 4;
    default:
        assert(0);
        return 0;
    }
}

static inline float um__inv_range(int type) {
    switch (type) {
    case UMUGU_TYPE_INT16:
        return 1.0f / 32768.0f;
    case UMUGU_TYPE_INT32:
        return 1.0f / 2147483647.0f;
    case UMUGU_TYPE_FLOAT:
        return 1.0f;
    default:
        assert(0);
        return 0;
    }
}

static inline int um__init(umugu_node *node) {
    umugu_ctx *ctx = umugu_get_context();
    um__wavplayer *self = (um__wavplayer *)node;
    node->pipe_out_ready = 0;
    node->pipe_out_type = UMUGU_PIPE_SIGNAL;

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
    self->source.channels = *(int16_t *)(header + 22);
    self->source.rate = *(int *)(header + 24);

    /* Here I'm deducing the type format given the byte size:
     *   - 1 byte : UINT8. UMUGU_TYPE_UINT8 = 64 so 63 + size is correct
     *   - 2 bytes: INT16. UMUGU_TYPE_INT16 = 65 so 63 + size is correct
     *   - 4 bytes: FLOAT. UMUGU_TYPE_FLOAT = 67 so 63 + size is correct */
    int sample_bytes = header[34] / 8;
    self->source.type = 63 + sample_bytes;

    /* The standard way would be checking first if it's int or float..
     * int type = header[20] == 1 ? INTEGER : FLOAT; // It can be 1 (int) or 3
     * (float) And then checking the size in bytes.. sample_bytes = header[34] /
     * 8; Note that in WAV format, integers of 8- bits are unsigned and 9+
     * signed.
     */
    self->source.capacity = UMUGU_DEFAULT_SAMPLE_CAPACITY;
    self->source.frames = ctx->alloc(self->source.capacity * sample_bytes *
                                     self->source.channels);
    self->source.count = 0;

    return UMUGU_SUCCESS;
}

/* TODO: Implement this node type mmapping the wav. */
static inline int um__process(umugu_node *node) {
    if (node->pipe_out_ready) {
        return UMUGU_NOOP;
    }

    um__wavplayer *self = (void *)node;
    self->source.count = umugu_get_context()->io.out_audio_signal.count;
    assert(self->source.count <= self->source.capacity);

    umugu_frame *out = umugu_get_temp_signal(&node->pipe_out);

    fread(self->source.frames,
          self->source.count * self->source.channels *
              um__getsize(self->source.type),
          1, (FILE *)self->file_handle);

    const float inv_range = um__inv_range(self->source.type);

    /* For now we're assuming that te wav is also 48000 samples rate
        and signed int16. */
    int16_t *it = (int16_t *)(self->source.frames);
    for (int i = 0; i < node->pipe_out.count; ++i) {
        if (self->source.channels == 2) {
            out[i].left = *it++ * inv_range;
            out[i].right = *it++ * inv_range;
        } else {
            out[i].left = *it * inv_range;
            out[i].right = *it++ * inv_range;
        }
    }

    node->pipe_out_ready = 1;

    return UMUGU_SUCCESS;
}

umugu_node_fn um__wavplayer_getfn(umugu_fn fn) {
    switch (fn) {
    case UMUGU_FN_INIT:
        return um__init;
    case UMUGU_FN_PROCESS:
        return um__process;
    default:
        return NULL;
    }
}
