#include "builtin_nodes.h"

/* Peak equalizer filter */
typedef struct {
    umugu_node node;
    float value;
    float gain;
    float freq;
    float center;
    float rec[3];
} um__peakeq;

const int32_t um__peakeq_size = (int32_t)sizeof(um__peakeq);
const int32_t um__peakeq_var_count = 3;

const umugu_var_info um__peakeq_vars[] = {
    {.name = {.str = "Gain"},
     .offset_bytes = offsetof(um__peakeq, gain),
     .type = UMUGU_TYPE_FLOAT,
     .count = 1,
     .misc.range.min = -10.0f,
     .misc.range.max = 10.0f},
    {.name = {.str = "Frequency"},
     .offset_bytes = offsetof(um__peakeq, freq),
     .type = UMUGU_TYPE_FLOAT,
     .count = 1,
     .misc.range.min = 100.0f,
     .misc.range.max = 10000.0f},
    {.name = {.str = "Center Freq"},
     .offset_bytes = offsetof(um__peakeq, center),
     .type = UMUGU_TYPE_FLOAT,
     .count = 1,
     .misc.range.min = 0.01f,
     .misc.range.max = 100.0f}};

static inline int um__init(umugu_node **node, umugu_signal *_) {
    um__peakeq *self = (void *)*node;
    *node = (void *)((char *)*node + sizeof(um__peakeq));
    umugu_node_call(UMUGU_FN_INIT, node, _);

    self->value = 6.2831855f / min(1.92e+05f, max(1.0f, UMUGU_SAMPLE_RATE));

    return UMUGU_SUCCESS;
}

static inline int um__getsignal(umugu_node **node, umugu_signal *out) {
    um__peakeq *self = (void *)*node;
    *node = (void *)((char *)*node + sizeof(um__peakeq));

    umugu_node_call(UMUGU_FN_GETSIGNAL, node, out);
    const int count = out->count;

    float slow0 = self->value * maxf(0.0f, self->gain);
    float slow1 = sinf(slow0);
    float slow2 = maxf(0.001f, self->freq);
    float slow3 = powf(1e+01f, 0.025f * self->center);
    float slow4 = 0.5f * (slow1 / (slow2 * slow3));
    float slow5 = 1.0f / (slow4 + 1.0f);
    float slow6 = 0.5f * (slow1 * slow3 / slow2);
    float slow7 = slow6 + 1.0f;
    float slow8 = 1.0f - slow4;
    float slow9 = 2.0f * cosf(slow0);
    float slow10 = 1.0f - slow6;

    for (int i = 0; i < count; ++i) {
        float temp = slow9 * self->rec[1];
        self->rec[0] = out->frames[i].left - slow5 * (slow8 * self->rec[2] - temp);
        out->frames[i].left = slow5 * (slow7 * self->rec[0] + slow10 * self->rec[2] - temp);
        self->rec[2] = self->rec[1];
        self->rec[1] = self->rec[0];

        temp = slow9 * self->rec[1];
        self->rec[0] = out->frames[i].right - slow5 * (slow8 * self->rec[2] - temp);
        out->frames[i].right = slow5 * (slow7 * self->rec[0] + slow10 * self->rec[2] - temp);
        self->rec[2] = self->rec[1];
        self->rec[1] = self->rec[0];
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
