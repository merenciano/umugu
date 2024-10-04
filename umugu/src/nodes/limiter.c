#include "umugu_internal.h"
#include <math.h>

static inline void um__init(umugu_node **node, umugu_signal *_)
{
    *node = (void*)((char*)*node + sizeof(um__limiter));
    umugu_node_call(UMUGU_FN_INIT, node, _);
}

static inline void um__getsignal(umugu_node **node, umugu_signal *out)
{
    um__limiter *self = (void*)*node;
    *node = (void*)((char*)*node + sizeof(um__limiter));

    umugu_node_call(UMUGU_FN_GETSIGNAL, node, out);
    const int count = out->count;
    for (int i = 0; i < count; ++i) {
        out->samples[i].left = fmin(out->samples[i].left, self->max);
        out->samples[i].left = fmax(out->samples[i].left, self->min);
        out->samples[i].right = fmin(out->samples[i].right, self->max);
        out->samples[i].right = fmax(out->samples[i].right, self->min);
    }
}

umugu_node_fn um__limiter_getfn(umugu_code fn)
{
    switch (fn)
    {
        case UMUGU_FN_INIT: return um__init;
        case UMUGU_FN_GETSIGNAL: return um__getsignal;
        default: return NULL;
    }
}
