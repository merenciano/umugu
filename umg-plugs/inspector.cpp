#include <stdio.h>
#include <umugu/umugu.h>

typedef struct Inspector {
    umugu_node node;
    int32_t it;
    int32_t padding;
    float* values;
    int32_t size;
    int32_t stride;
    int32_t offset;
    int32_t pause;
} Inspector;

extern "C" const size_t size = sizeof(Inspector);
extern "C" umugu_node_fn getfn(umugu_code fn);

static void Init(umugu_node** node, umugu_signal* _)
{
    Inspector* self = (Inspector*)*node;
    *node = (umugu_node*)((char*)*node + size);
    umugu_node_call(UMUGU_FN_INIT, node, _);
    self->it = 0;
    self->offset = 0;
    self->stride = 2;
    self->size = 2048;
    self->pause = false;
    self->values = (float*)(*umugu_get_context()->alloc)(self->size * self->stride * sizeof(float));
}

static void GetSignal(umugu_node** node, umugu_signal* out)
{
    Inspector* self = (Inspector*)*node;
    *node = (umugu_node*)((char*)*node + size);

    umugu_node_call(UMUGU_FN_GETSIGNAL, node, out);
    if (self->pause) {
        return;
    }

    float* fout = (float*)out->samples;
    const int count = out->count;
    for (int i = self->offset; i < count * 2; i += self->stride) {
        self->values[self->it] = fout[i];
        self->values[self->it + self->size] = fout[i];
        self->it++;
        if (self->it >= self->size) {
            self->it -= self->size;
        }
    }
}

umugu_node_fn getfn(umugu_code fn)
{
    switch (fn) {
    case UMUGU_FN_INIT:
        return Init;
    case UMUGU_FN_GETSIGNAL:
        return GetSignal;
    default:
        return NULL;
    }
}