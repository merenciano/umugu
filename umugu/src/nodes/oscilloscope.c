#include "builtin_nodes.h"
#include "umugu.h"

#include <string.h>

typedef struct {
    umugu_node node;
    int32_t phase;
    int32_t freq;
    int32_t waveform;
    int32_t sample_capacity;
    umugu_frame *audio_source;
} um__oscope;

const int32_t um__oscope_size = (int32_t)sizeof(um__oscope);
const int32_t um__oscope_var_count = 2;

const umugu_var_info um__oscope_vars[] = {
    {.name = {.str = "Frequency"},
     .offset_bytes = offsetof(um__oscope, freq),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .misc.range.min = 1,
     .misc.range.max = 8372},
    {.name = {.str = "Waveform"},
     .offset_bytes = offsetof(um__oscope, waveform),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .misc.range.min = 0,
     .misc.range.max = UM__WAVEFORM_COUNT}};

static inline int um__init(umugu_node **node, umugu_signal *_) {
    (void)_;
    umugu_ctx *ctx = umugu_get_context();
    um__oscope *self = (void *)*node;
    *node = (void *)((char *)*node + sizeof(um__oscope));
    self->sample_capacity = UMUGU_DEFAULT_SAMPLE_CAPACITY;
    self->audio_source =
        ctx->alloc(self->sample_capacity * sizeof(umugu_frame));
    return UMUGU_SUCCESS;
}

static inline int um__getsignal(umugu_node **node, umugu_signal *out) {
    umugu_ctx *ctx = umugu_get_context();
    um__oscope *self = (void *)*node;
    *node = (void *)((char *)*node + sizeof(um__oscope));
    const int count = out->count;

    if (!self->freq) {
        memset(out->frames, 0, count * sizeof(umugu_frame));
        return UMUGU_SUCCESS;
    }

    if (count > self->sample_capacity) {
        self->sample_capacity *= 2;
        if (self->sample_capacity < 256) {
            self->sample_capacity = 256;
        }

        ctx->free(self->audio_source);
        self->audio_source =
            ctx->alloc(self->sample_capacity * sizeof(umugu_frame));
        if (!self->audio_source) {
            ctx->io.log("Error (oscilloscope): BadAlloc.\n");
            self->sample_capacity = 0;
            self->audio_source = NULL;
            return UMUGU_ERR_MEM;
        }
    }

    for (int i = 0; i < count; i++) {
        float sample = um__waveform_lut[self->waveform][self->phase];
        out->frames[i].left = sample;
        out->frames[i].right = sample;
        self->phase += self->freq;
        if (self->phase >= UMUGU_SAMPLE_RATE) {
            self->phase -= UMUGU_SAMPLE_RATE;
        }
    }
    //out->frames = self->audio_source;
    return UMUGU_SUCCESS;
}

umugu_node_fn um__oscope_getfn(umugu_fn fn) {
    switch (fn) {
    case UMUGU_FN_INIT:
        return um__init;
    case UMUGU_FN_GETSIGNAL:
        return um__getsignal;
    default:
        return NULL;
    }
}
