#include <umugu/umugu.h>

#include <math.h>

struct Acid {
  umugu_node Node;
  float Cutoff;
  float Resonance;
  float Buf0;
  float Buf1;
  float Drive;
  float Mix;
};

static const umugu_var_info var_metadata[] = {{.name = {.str = "Cutoff"},
                                               .offset_bytes = offsetof(Acid, Cutoff),
                                               .type = UMUGU_TYPE_FLOAT,
                                               .count = 1,
                                               .flags = UMUGU_VAR_NONE,
                                               .misc = {.rangef = {.min = 0.0f, .max = 1.0f}}},
                                              {.name = {.str = "Resonance"},
                                               .offset_bytes = offsetof(Acid, Resonance),
                                               .type = UMUGU_TYPE_FLOAT,
                                               .count = 1,
                                               .flags = UMUGU_VAR_NONE,
                                               .misc = {.rangef = {.min = 0.0f, .max = 1.0f}}},
                                              {.name = {.str = "Drive"},
                                               .offset_bytes = offsetof(Acid, Drive),
                                               .type = UMUGU_TYPE_FLOAT,
                                               .count = 1,
                                               .flags = UMUGU_VAR_NONE,
                                               .misc = {.rangef = {.min = 0.0f, .max = 5.0f}}},
                                              {.name = {.str = "Mix"},
                                               .offset_bytes = offsetof(Acid, Mix),
                                               .type = UMUGU_TYPE_FLOAT,
                                               .count = 1,
                                               .flags = UMUGU_VAR_NONE,
                                               .misc = {.rangef = {.min = 0.0f, .max = 1.0f}}}};

extern "C" const int32_t size = (int32_t)sizeof(Acid);
extern "C" umugu_node_fn getfn(int32_t fn);
extern "C" const umugu_var_info *vars;
extern "C" const int32_t var_count = 4;

const umugu_var_info *vars = &var_metadata[0];

static int Init(umugu_node *apNode) {
  Acid *pSelf = (Acid *)apNode;
  pSelf->Buf0 = 0.0f;
  pSelf->Buf1 = 0.0f;
  return UMUGU_SUCCESS;
}

static inline umugu_frame Distortion(umugu_frame aInput, Acid &aSelf) {
  aInput.left *= aSelf.Drive;
  aInput.right *= aSelf.Drive;
  return {.left = aInput.left > 0.0f ? 1.0f - expf(-aInput.left) : -1.0f + expf(aInput.left),
          .right = aInput.right > 0.0f ? 1.0f - expf(-aInput.right) : -1.0f + expf(aInput.right)};
}

static int Process(umugu_node *apNode) {
  if (apNode->pipe_out_ready) {
    return UMUGU_NOOP;
  }

  Acid *pSelf = (Acid *)apNode;

  auto pOut = umugu_get_temp_signal(&apNode->pipe_out);
  const int Count = apNode->pipe_out.count;
  // float Fb = pSelf->Resonance + pSelf->Resonance / (1.0f - pSelf->Cutoff);

  for (int i = 0; i < Count; i++) {
#if 0
    pSelf->Buf0 +=
        pSelf->Cutoff * (apOut->frames[i].left - pSelf->Buf0 + Fb * (pSelf->Buf0 - pSelf->Buf1));
    pSelf->Buf1 += pSelf->Cutoff * (pSelf->Buf0 - pSelf->Buf1);
    apOut->frames[i].left = pSelf->Buf1;
    apOut->frames[i].right = pSelf->Buf1;
    printf("%f, %f \n", apOut->frames[i].left, apOut->frames[i].right);
#endif
    auto Frame = Distortion(pOut[i], *pSelf);
    pOut[i].left = pOut[i].left * (1.0f - pSelf->Mix) + Frame.left * pSelf->Mix;
    pOut[i].right = pOut[i].right * (1.0f - pSelf->Mix) + Frame.right * pSelf->Mix;
  }
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
