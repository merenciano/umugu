#include <umugu/umugu.h>

struct Inspector {
  umugu_node Node;
  float *OutValues;
  int32_t It;
  int32_t Padding;
  float *Values;
  int32_t Size;
  int32_t Stride;
  int32_t Offset;
  int32_t Pause;
};

#ifndef offsetof
#define offsetof(t, d) __builtin_offsetof(t, d)
#endif

static const umugu_var_info var_metadata[] = {
    {.name = {.str = "Values"},
     .offset_bytes = offsetof(Inspector, OutValues),
     .type = UMUGU_TYPE_PLOTLINE,
     .count = 2048,
     .misc = {.range = {.min = 0, .max = 2048}}},
    {.name = {.str = "Stride"},
     .offset_bytes = offsetof(Inspector, Stride),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .misc = {.range = {.min = 1, .max = 1024}}},
    {.name = {.str = "Offset"},
     .offset_bytes = offsetof(Inspector, Offset),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .misc = {.range = {.min = 1, .max = 1024}}},
    {.name = {.str = "Pause"},
     .offset_bytes = offsetof(Inspector, Pause),
     .type = UMUGU_TYPE_BOOL,
     .count = 1,
     .misc = {.range = {.min = 0, .max = 1}}},
};

extern "C" const int32_t size = (int32_t)sizeof(Inspector);
extern "C" umugu_node_fn getfn(int32_t fn);
extern "C" const umugu_var_info *vars;
extern "C" const int32_t var_count = 4;

const umugu_var_info *vars = &var_metadata[0];

static int Init(umugu_node **apNode, umugu_signal *_) {
  Inspector *pSelf = (Inspector *)*apNode;
  *apNode = (umugu_node *)((char *)*apNode + size);
  umugu_node_call(UMUGU_FN_INIT, apNode, _);
  pSelf->It = 0;
  pSelf->Offset = 0;
  pSelf->Stride = 2;
  pSelf->Size = 2048;
  pSelf->Pause = false;
  pSelf->Values = (float *)(*umugu_get_context()->alloc)(pSelf->Size * 2 * sizeof(float));
  pSelf->OutValues = pSelf->Values;
  return UMUGU_SUCCESS;
}

static int GetSignal(umugu_node **apNode, umugu_signal *apOut) {
  Inspector *pSelf = (Inspector *)*apNode;
  *apNode = (umugu_node *)((char *)*apNode + size);

  umugu_node_call(UMUGU_FN_GETSIGNAL, apNode, apOut);
  if (pSelf->Pause) {
    return UMUGU_SUCCESS;
  }

  float *pOutFloat = (float *)apOut->frames;
  const int Count = apOut->count;
  for (int i = pSelf->Offset; i < Count * 2; i += pSelf->Stride) {
    pSelf->Values[pSelf->It] = pOutFloat[i];
    pSelf->Values[pSelf->It + pSelf->Size] = pOutFloat[i];
    pSelf->It++;
    if (pSelf->It >= pSelf->Size) {
      pSelf->It -= pSelf->Size;
    }
  }
  pSelf->OutValues = pSelf->Values + pSelf->It;
  return UMUGU_SUCCESS;
}

umugu_node_fn getfn(int32_t fn) {
  switch (fn) {
  case UMUGU_FN_INIT:
    return Init;
  case UMUGU_FN_GETSIGNAL:
    return GetSignal;
  default:
    return nullptr;
  }
}
