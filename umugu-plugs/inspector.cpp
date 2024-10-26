#include <umugu/umugu.h>

#include <assert.h>
#include <string.h>

constexpr int32_t ValuesBufferSize = 2048;

struct Inspector {
  umugu_node Node;
  float *OutValues;
  int32_t It;
  int32_t Padding;
  int32_t Size = ValuesBufferSize;
  int32_t Stride;
  int32_t Offset;
  int32_t Pause;
  float Values[ValuesBufferSize * UMUGU_CHANNELS * sizeof(float)];
};

static const umugu_var_info var_metadata[] = {
    {.name = {.str = "Values"},
     .offset_bytes = offsetof(Inspector, OutValues),
     .type = UMUGU_TYPE_PLOTLINE,
     .count = 2048,
     .flags = UMUGU_VAR_RDONLY,
     .misc = {.rangei = {.min = 0, .max = 2048}}},
    {.name = {.str = "Stride"},
     .offset_bytes = offsetof(Inspector, Stride),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .flags = UMUGU_VAR_NONE,
     .misc = {.rangei = {.min = 1, .max = 1024}}},
    {.name = {.str = "Offset"},
     .offset_bytes = offsetof(Inspector, Offset),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .flags = UMUGU_VAR_NONE,
     .misc = {.rangei = {.min = 1, .max = 1024}}},
    {.name = {.str = "Pause"},
     .offset_bytes = offsetof(Inspector, Pause),
     .type = UMUGU_TYPE_BOOL,
     .count = 1,
     .flags = UMUGU_VAR_NONE,
     .misc = {.rangei = {.min = 0, .max = 1}}},
};

extern "C" const int32_t size = (int32_t)sizeof(Inspector);
extern "C" umugu_node_fn getfn(int32_t fn);
extern "C" const umugu_var_info *vars;
extern "C" const int32_t var_count = 4;

const umugu_var_info *vars = &var_metadata[0];

static int Init(umugu_node *apNode) {
  Inspector *pSelf = (Inspector *)apNode;
  pSelf->It = 0;
  pSelf->Offset = 0;
  pSelf->Stride = UMUGU_CHANNELS;
  pSelf->Size = ValuesBufferSize;
  pSelf->Pause = false;
  pSelf->OutValues = pSelf->Values;
  apNode->pipe_out_ready = false;
  memset(pSelf->Values, 0, sizeof(pSelf->Values));
  return UMUGU_SUCCESS;
}

static int Process(umugu_node *apNode) {
  if (apNode->pipe_out_ready) {
    return UMUGU_NOOP;
  }

  auto pCtx = umugu_get_context();
  Inspector *pSelf = (Inspector *)apNode;
  auto pInputNode = pCtx->pipeline.nodes[apNode->pipe_in_node_idx];
  if (!pInputNode->pipe_out_ready) {
    umugu_node_dispatch(apNode, UMUGU_FN_PROCESS);
    assert(pInputNode->pipe_out_ready);
  }

  if (pSelf->Pause) {
    return UMUGU_SUCCESS;
  }

  float *pInputFloat = (float *)pInputNode->pipe_out.frames;
  const int Count = pInputNode->pipe_out.count;
  for (int i = pSelf->Offset; i < Count * 2; i += pSelf->Stride) {
    pSelf->Values[pSelf->It] = pInputFloat[i];
    pSelf->Values[pSelf->It + pSelf->Size] = pInputFloat[i];
    pSelf->It++;
    if (pSelf->It >= pSelf->Size) {
      pSelf->It -= pSelf->Size;
    }
  }
  pSelf->OutValues = pSelf->Values + pSelf->It;
  apNode->pipe_out = pInputNode->pipe_out;
  apNode->pipe_out_ready = true;
  return UMUGU_SUCCESS;
}

umugu_node_fn getfn(int32_t fn) {
  switch (fn) {
  case UMUGU_FN_INIT:
    return Init;
  case UMUGU_FN_PROCESS:
    return Process;
  default:
    return nullptr;
  }
}
