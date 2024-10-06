#include "builtin_nodes.h"
#include "umugu.h"

typedef struct {
    umugu_node node;
    float multiplier;
    int32_t padding;
} um__amplitude;

const int32_t um__amplitude_size = (int32_t)sizeof(um__amplitude);
const int32_t um__amplitude_var_count = 1;

const umugu_var_info um__amplitude_vars[] = {
    { .name = {.str = "Multiplier"},
      .offset_bytes = offsetof(um__amplitude, multiplier),
      .type = UMUGU_TYPE_FLOAT,
      .count = 1,
      .misc.range.min = 0.0f,
      .misc.range.max = 5.0f
    }
};

static inline int um__init(umugu_node **node, umugu_signal *_)
{
    *node = (void*)((char*)*node + sizeof(um__amplitude));
    umugu_node_call(UMUGU_FN_INIT, node, _);
    return UMUGU_SUCCESS;
}

static inline int um__getsignal(umugu_node **node, umugu_signal *out)
{
    um__amplitude *self = (void*)*node;
    *node = (void*)((char*)*node + sizeof(um__amplitude));

    umugu_node_call(UMUGU_FN_GETSIGNAL, node, out);
    const int count = out->count;
    for (int i = 0; i < count; ++i) {
        out->frames[i].left *= self->multiplier;
        out->frames[i].right *= self->multiplier;
    }
    return UMUGU_SUCCESS;
}

umugu_node_fn um__amplitude_getfn(umugu_fn fn)
{
    switch (fn)
    {
        case UMUGU_FN_INIT: return um__init;
        case UMUGU_FN_GETSIGNAL: return um__getsignal;
        default: return NULL;
    }
}
