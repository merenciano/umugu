#include "umugu_internal.h"

static inline void um__init(umugu_node **node, umugu_signal *_)
{
    um__mixer *self = (void*)*node;
    *node = (void*)((char*)*node + sizeof(um__mixer));
    umugu_node_call(UMUGU_FN_INIT, node, _);
    for (int i = 1; i < self->input_count; ++i) {
        umugu_node_call(UMUGU_FN_INIT, node, _);
    }
}

static inline void um__getsignal(umugu_node **node, umugu_signal *out)
{
    um__mixer *self = (void*)*node;
    *node = (void*)((char*)*node + sizeof(um__mixer));

    umugu_node_call(UMUGU_FN_GETSIGNAL, node, out);
    const int sample_count = out->count;
    for (int i = 1; i < self->input_count; ++i) {
        umugu_signal tmp = {.count = sample_count};
        umugu_node_call(UMUGU_FN_GETSIGNAL, node, &tmp);
        for (int j = 0; j < sample_count; ++j) {
            out->samples[j].left += tmp.samples[j].left;
            out->samples[j].right += tmp.samples[j].right;
        }
    }

    float inv_count = 1.0f / self->input_count;
    for (int i = 0; i < sample_count; i++) {
        out->samples[i].left *= inv_count;
        out->samples[i].right *= inv_count;
    }
}

umugu_node_fn um__mixer_getfn(umugu_code fn)
{
    switch (fn)
    {
        case UMUGU_FN_INIT: return um__init;
        case UMUGU_FN_GETSIGNAL: return um__getsignal;
        default: return NULL;
    }
}
