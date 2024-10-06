#include "builtin_nodes.h"
#include "umugu.h"

typedef struct {
    umugu_node node;
    int32_t input_count;
    int32_t padding;
} um__mixer;

const int32_t um__mixer_size = (int32_t)sizeof(um__mixer);
const int32_t um__mixer_var_count = 1;

const umugu_var_info um__mixer_vars[] = {
    {
        .name = {.str = "InputCount"},
        .offset_bytes = offsetof(um__mixer, input_count),
        .type = UMUGU_TYPE_INT32,
        .count = 1,
        .misc.range.min = 0,
        .misc.range.max = 8
    }
};

static inline int um__init(umugu_node **node, umugu_signal *_)
{
    um__mixer *self = (void*)*node;
    *node = (void*)((char*)*node + sizeof(um__mixer));
    umugu_node_call(UMUGU_FN_INIT, node, _);
    for (int i = 1; i < self->input_count; ++i) {
        umugu_node_call(UMUGU_FN_INIT, node, _);
    }
    return UMUGU_SUCCESS;
}

static inline int um__getsignal(umugu_node **node, umugu_signal *out)
{
    um__mixer *self = (void*)*node;
    *node = (void*)((char*)*node + sizeof(um__mixer));

    umugu_node_call(UMUGU_FN_GETSIGNAL, node, out);
    const int sample_count = out->count;
    for (int i = 1; i < self->input_count; ++i) {
        umugu_signal tmp = {.count = sample_count};
        umugu_node_call(UMUGU_FN_GETSIGNAL, node, &tmp);
        for (int j = 0; j < sample_count; ++j) {
            out->frames[j].left += tmp.frames[j].left;
            out->frames[j].right += tmp.frames[j].right;
        }
    }

    float inv_count = 1.0f / self->input_count;
    for (int i = 0; i < sample_count; i++) {
        out->frames[i].left *= inv_count;
        out->frames[i].right *= inv_count;
    }
    return UMUGU_SUCCESS;
}

umugu_node_fn um__mixer_getfn(umugu_fn fn)
{
    switch (fn)
    {
        case UMUGU_FN_INIT: return um__init;
        case UMUGU_FN_GETSIGNAL: return um__getsignal;
        default: return NULL;
    }
}
