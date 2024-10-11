#include "builtin_nodes.h"

#include <math.h>
#include <stdio.h>

#define UMUGU_BUTTERWORTH_G 0.7071f

typedef struct {
    umugu_node node;
    int32_t sample_rate;
    float cutoff; /* Hz */
    /* Filter values */
    float a[3];
    float b[3];
    /* Previous in / out values  */
    float x[UMUGU_CHANNELS][3];
    float y[UMUGU_CHANNELS][3];
} um__butterworth;

const int32_t um__butterworth_size = (int32_t)sizeof(um__butterworth);
const int32_t um__butterworth_var_count = 2;

const umugu_var_info um__butterworth_vars[] = {
    {.name = {.str = "SampleRate"},
     .offset_bytes = offsetof(um__butterworth, sample_rate),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .misc.range.min = 6000.0f,
     .misc.range.max = 96000.0f},
    {.name = {.str = "Cutoff(Hz)"},
     .offset_bytes = offsetof(um__butterworth, cutoff),
     .type = UMUGU_TYPE_FLOAT,
     .count = 1,
     .misc.range.min = 0.0f,
     .misc.range.max = 22000.0f}};

static inline int um__init(umugu_node **node, umugu_signal *_) {
    um__butterworth *self = (void *)*node;
    *node = (umugu_node*)(self + 1);
    umugu_node_call(UMUGU_FN_INIT, node, _);

    float w0 = M_PI_2 * self->cutoff / (float)self->sample_rate;
    float alpha = sinf(w0) / (2.0f * UMUGU_BUTTERWORTH_G);

    self->b[0] = (1.0f - cosf(w0)) / 2.0f;
    self->b[1] = 1.0f - cosf(w0);
    self->b[2] = (1.0f - cosf(w0)) / 2.0f;
    self->a[0] = 1.0f + alpha;
    self->a[1] = -2.0f * cosf(w0);
    self->a[2] = 1.0f - alpha;

    /* Normalize coefficients. */
    const float recip = 1.0f / self->a[0];
    self->b[0] *= recip;
    self->a[0] *= recip;
    self->b[1] *= recip;
    self->a[1] *= recip;
    self->b[2] *= recip;
    self->a[2] *= recip;

    for (int i = 0; i < 3; ++i) {
        printf("A %d = %f\n", i, self->a[i]);
        printf("B %d = %f\n", i, self->b[i]);
    }

    /* Default state */
    for (int i = 0; i < UMUGU_CHANNELS; ++i) {
        self->x[i][0] = 1.0f;
        self->y[i][0] = 1.0f;
        self->x[i][1] = 1.0f;
        self->y[i][1] = 1.0f;
        self->x[i][2] = 1.0f;
        self->y[i][2] = 1.0f;
    }

    return UMUGU_SUCCESS;
}

static inline int um__getsignal(umugu_node **node, umugu_signal *out) {
    um__butterworth *self = (void *)*node;
    *node = (umugu_node*)(self + 1);
    umugu_node_call(UMUGU_FN_GETSIGNAL, node, out);

    const int count = out->count;
    for (int i = 0; i < count; ++i) {
        /* Input state step */
        self->x[0][2] = self->x[0][1];
        self->x[0][1] = self->x[0][0];
        self->x[0][0] = out->frames[i].left;

        self->x[1][2] = self->x[1][1];
        self->x[1][1] = self->x[1][0];
        self->x[1][0] = out->frames[i].right;

        /* Filter */
        out->frames[i].left =
            self->b[0] * self->x[0][0] +
            self->b[1] * self->x[0][1] +
            self->b[2] * self->x[0][2] - 
            self->a[1] * self->y[0][1] -
            self->a[2] * self->y[0][2];
        out->frames[i].right =
            self->b[0] * self->x[1][0] +
            self->b[1] * self->x[1][1] +
            self->b[2] * self->x[1][2] - 
            self->a[1] * self->y[1][1] -
            self->a[2] * self->y[1][2];
        
        /* Output state step */
        self->y[0][2] = self->y[0][1];
        self->y[0][1] = self->y[0][0];
        self->y[0][0] = out->frames[i].left;

        self->y[1][2] = self->y[1][1];
        self->y[1][1] = self->y[1][0];
        self->y[1][0] = out->frames[i].right;
    }
    return UMUGU_SUCCESS;
}

umugu_node_fn um__butterworth_getfn(umugu_fn fn) {
    switch (fn) {
    case UMUGU_FN_INIT:
        return um__init;
    case UMUGU_FN_GETSIGNAL:
        return um__getsignal;
    default:
        return NULL;
    }
}
