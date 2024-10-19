#include "builtin_nodes.h"
#include "umugu.h"
#include <string.h>

typedef struct {
    umugu_node node;
    int32_t input_count;
    int32_t padding;
} um__mixer;

const int32_t um__mixer_size = (int32_t)sizeof(um__mixer);
const int32_t um__mixer_var_count = 1;

const umugu_var_info um__mixer_vars[] = {
    {.name = {.str = "InputCount"},
     .offset_bytes = offsetof(um__mixer, input_count),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .misc.range.min = 0,
     .misc.range.max = 32}};

static inline int um__init(umugu_node **node, umugu_signal *_) {
    um__mixer *self = (void *)*node;
    *node = (void *)((char *)*node + sizeof(um__mixer));
    umugu_node_call(UMUGU_FN_INIT, node, _);
    for (int i = 1; i < self->input_count; ++i) {
        umugu_node_call(UMUGU_FN_INIT, node, _);
    }
    return UMUGU_SUCCESS;
}

static umugu_frame frames_buffer[1024];
static umugu_frame frames_empty[256];

static inline int um__getsignal(umugu_node **node, umugu_signal *out) {
    um__mixer *self = (void *)*node;
    *node = (void *)((char *)*node + sizeof(um__mixer));

    int signals = 0;
    umugu_node_call(UMUGU_FN_GETSIGNAL, node, out);
    if (memcmp(out->frames, frames_empty, out->count < 256 ? out->count : 256)) {
        ++signals;
    }
    const int sample_count = out->count;
    umugu_signal tmp = {.frames = frames_buffer, out->count, out->rate, out->type, out->channels, 1024};
    for (int i = 1; i < self->input_count; ++i) {
        umugu_node_call(UMUGU_FN_GETSIGNAL, node, &tmp);
        if (memcmp(tmp.frames, frames_empty, tmp.count < 256 ? tmp.count : 256)) {
            ++signals;
        }

        for (int j = 0; j < sample_count; ++j) {
            out->frames[j].left += tmp.frames[j].left;
            out->frames[j].right += tmp.frames[j].right;
        }
    }

    float inv_count = 1.0f / signals;
    for (int i = 0; i < sample_count; i++) {
        out->frames[i].left *= inv_count;
        out->frames[i].right *= inv_count;
    }

    return UMUGU_SUCCESS;
}

umugu_node_fn um__mixer_getfn(umugu_fn fn) {
    switch (fn) {
    case UMUGU_FN_INIT:
        return um__init;
    case UMUGU_FN_GETSIGNAL:
        return um__getsignal;
    default:
        return NULL;
    }
}
