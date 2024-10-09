#include <stddef.h>
#include <umugu/umugu.h>

struct Inspector {
  umugu_node node;
  float *out_values;
  int32_t it;
  int32_t padding;
  float *values;
  int32_t size;
  int32_t stride;
  int32_t offset;
  int32_t pause;
};

static const umugu_var_info var_metadata[] = {
    {.name = {.str = "Values"},
     .offset_bytes = offsetof(Inspector, out_values),
     .type = UMUGU_TYPE_PLOTLINE,
     .count = 2048,
     .misc = {.range = {.min = 0, .max = 2048}}},
    {.name = {.str = "Stride"},
     .offset_bytes = offsetof(Inspector, stride),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .misc = {.range = {.min = 1, .max = 1024}}},
    {.name = {.str = "Offset"},
     .offset_bytes = offsetof(Inspector, offset),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .misc = {.range = {.min = 1, .max = 1024}}},
    {.name = {.str = "Pause"},
     .offset_bytes = offsetof(Inspector, pause),
     .type = UMUGU_TYPE_BOOL,
     .count = 1,
     .misc = {.range = {.min = 0, .max = 1}}},
};

extern "C" const int32_t size = (int32_t)sizeof(Inspector);
extern "C" umugu_node_fn getfn(int32_t fn);
extern "C" const umugu_var_info *vars;
extern "C" const int32_t var_count = 4;

const umugu_var_info *vars = &var_metadata[0];

static int Init(umugu_node **node, umugu_signal *_) {
  Inspector *self = (Inspector *)*node;
  *node = (umugu_node *)((char *)*node + size);
  umugu_node_call(UMUGU_FN_INIT, node, _);
  self->it = 0;
  self->offset = 0;
  self->stride = 2;
  self->size = 2048;
  self->pause = false;
  self->values =
      (float *)(*umugu_get_context()->alloc)(self->size * 2 * sizeof(float));
  self->out_values = self->values;
  return UMUGU_SUCCESS;
}

static int GetSignal(umugu_node **node, umugu_signal *out) {
  Inspector *self = (Inspector *)*node;
  *node = (umugu_node *)((char *)*node + size);

  umugu_node_call(UMUGU_FN_GETSIGNAL, node, out);
  if (self->pause) {
    return UMUGU_SUCCESS;
  }

  float *fout = (float *)out->frames;
  const int count = out->count;
  for (int i = self->offset; i < count * 2; i += self->stride) {
    self->values[self->it] = fout[i];
    self->values[self->it + self->size] = fout[i];
    self->it++;
    if (self->it >= self->size) {
      self->it -= self->size;
    }
  }
  self->out_values = self->values + self->it;
  return UMUGU_SUCCESS;
}

umugu_node_fn getfn(int32_t fn) {
  switch (fn) {
  case UMUGU_FN_INIT:
    return Init;
  case UMUGU_FN_GETSIGNAL:
    return GetSignal;
  default:
    return NULL;
  }
}