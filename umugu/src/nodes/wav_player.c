#include "builtin_nodes.h"
#include "umugu.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static inline int um__getsize(int type) {
    switch (type) {
    case UMUGU_TYPE_UINT8:
        return 1;
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
    case UMUGU_TYPE_UINT8:
        return 1.0f / 255.0f;
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
    node->out_pipe_ready = 0;
    node->out_pipe_type = UMUGU_PIPE_SIGNAL;

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
    self->channels = *(int16_t *)(header + 22);
    self->sample_rate = *(int *)(header + 24);

    /* Here I'm deducing the type format given the byte size:
     *   - 1 byte : UINT8. UMUGU_TYPE_UINT8 = 64 so 63 + size is correct
     *   - 2 bytes: INT16. UMUGU_TYPE_INT16 = 65 so 63 + size is correct
     *   - 4 bytes: FLOAT. UMUGU_TYPE_FLOAT = 67 so 63 + size is correct */
    self->sample_size_bytes = header[34] / 8;
    self->sample_type = 63 + self->sample_size_bytes;

    /* The standard way would be checking first if it's int or float..
     * int type = header[20] == 1 ? INTEGER : FLOAT; // It can be 1 (int) or 3
     * (float) And then checking the size in bytes.. sample_bytes = header[34] /
     * 8; Note that in WAV format, integers of 8- bits are unsigned and 9+
     * signed.
     */

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
    if (node->out_pipe_ready) {
        return UMUGU_NOOP;
    }

    um__wavplayer *self = (void *)node;
    // umugu_frame *out = umugu_get_temp_signal(&node->pipe_out);
    const int count = umugu_get_context()->io.out_audio_signal.count;
    const size_t signal_size =
        count * self->channels * um__getsize(self->sample_type);
    umugu_frame *src = umugu_alloc_temp(signal_size);
    fread(src, signal_size, 1, self->file_handle);
    const float inv_range = um__inv_range(self->sample_type);

    /* For now we're assuming that te wav is also 48000 samples rate
        and signed int16.
        TODO: Implement UINT8, INT32 and FLOAT at least. */
    if (self->sample_rate != 48000 || self->sample_type != UMUGU_TYPE_INT16) {
        umugu_get_context()->io.log("[ERR] WavPlayer: sample rate and sample "
                                    "type conversions not supported yet.");
        return UMUGU_ERR_STREAM;
    }

    umugu_frame *out = umugu_get_temp_signal(&node->out_pipe);
    int16_t *it = (int16_t *)(src);
    for (int i = 0; i < node->out_pipe.count; ++i) {
        if (self->channels == 2) {
            out[i].left = *it++ * inv_range;
            out[i].right = *it++ * inv_range;
        } else {
            out[i].left = *it * inv_range;
            out[i].right = *it++ * inv_range;
        }
    }

    node->out_pipe_ready = 1;

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
