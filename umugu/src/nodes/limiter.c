#include "builtin_nodes.h"

#include <math.h>

typedef struct {
    umugu_node node;
    float min;
    float max;
} um__limiter;

const int32_t um__limiter_size = (int32_t)sizeof(um__limiter);
const int32_t um__limiter_var_count = 2;

const umugu_var_info um__limiter_vars[] = {
    {.name = {.str = "Min"},
     .offset_bytes = offsetof(um__limiter, min),
     .type = UMUGU_TYPE_FLOAT,
     .count = 1,
     .misc.range.min = -5.0f,
     .misc.range.max = 5.0f},
    {.name = {.str = "Max"},
     .offset_bytes = offsetof(um__limiter, max),
     .type = UMUGU_TYPE_FLOAT,
     .count = 1,
     .misc.range.min = -5.0f,
     .misc.range.max = 5.0f}};

static inline int um__init(umugu_node **node, umugu_signal *_) {
    *node = (void *)((char *)*node + sizeof(um__limiter));
    umugu_node_call(UMUGU_FN_INIT, node, _);
    return UMUGU_SUCCESS;
}

static inline int um__getsignal(umugu_node **node, umugu_signal *out) {
    um__limiter *self = (void *)*node;
    *node = (void *)((char *)*node + sizeof(um__limiter));

    umugu_node_call(UMUGU_FN_GETSIGNAL, node, out);
    const int count = out->count;
    for (int i = 0; i < count; ++i) {
        out->frames[i].left = fmin(out->frames[i].left, self->max);
        out->frames[i].left = fmax(out->frames[i].left, self->min);
        out->frames[i].right = fmin(out->frames[i].right, self->max);
        out->frames[i].right = fmax(out->frames[i].right, self->min);
    }
    return UMUGU_SUCCESS;
}

umugu_node_fn um__limiter_getfn(umugu_fn fn) {
    switch (fn) {
    case UMUGU_FN_INIT:
        return um__init;
    case UMUGU_FN_GETSIGNAL:
        return um__getsignal;
    default:
        return NULL;
    }
}
