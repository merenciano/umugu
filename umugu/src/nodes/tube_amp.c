#include "builtin_nodes.h"

#include <math.h>

typedef struct {
    umugu_node node;
    float gain;
} um__tubeamp;

const int32_t um__tubeamp_size = (int32_t)sizeof(um__tubeamp);
const int32_t um__tubeamp_var_count = 1;

const umugu_var_info um__tubeamp_vars[] = {
    {.name = {.str = "Gain"},
     .offset_bytes = offsetof(um__tubeamp, gain),
     .type = UMUGU_TYPE_FLOAT,
     .count = 1,
     .misc.range.min = -50.0f,
     .misc.range.max = 50.0f}
};

static inline int um__init(umugu_node **node, umugu_signal *_) {
    *node = (void *)((char *)*node + sizeof(um__tubeamp));
    umugu_node_call(UMUGU_FN_INIT, node, _);
    return UMUGU_SUCCESS;
}

static inline int um__getsignal(umugu_node **node, umugu_signal *out) {
    um__tubeamp *self = (void *)*node;
    *node = (void *)((char *)*node + sizeof(um__tubeamp));

    umugu_node_call(UMUGU_FN_GETSIGNAL, node, out);

    const float atan_gain = atan(self->gain);
    const int count = out->count;
    for (int i = 0; i < count; ++i) {
        out->frames[i].left = atan(self->gain * out->frames[i].left) / atan_gain;
        out->frames[i].right = atan(self->gain * out->frames[i].right) / atan_gain;
    }
    return UMUGU_SUCCESS;
}

umugu_node_fn um__tubeamp_getfn(umugu_fn fn) {
    switch (fn) {
    case UMUGU_FN_INIT:
        return um__init;
    case UMUGU_FN_GETSIGNAL:
        return um__getsignal;
    default:
        return NULL;
    }
}
