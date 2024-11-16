/*
 * Used to test user-defined nodes written in c++.
 */

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
    float Values[ValuesBufferSize * 2 * sizeof(float)];
};

static const umugu_var_info metadata[] = {
    {.name = {.str = "Values"},
     .offset_bytes = offsetof(Inspector, OutValues),
     .type = UMUGU_TYPE_FLOAT,
     .count = 2048,
     .flags = UMUGU_VAR_RDONLY | UMUGU_VAR_PLOTLINE,
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
extern "C" const int32_t var_count = sizeof(metadata) / sizeof(*metadata);

const umugu_var_info *vars = &metadata[0];

static int Init(umugu_node *apNode) {
    Inspector *pSelf = (Inspector *)apNode;
    pSelf->It = 0;
    pSelf->Offset = 0;
    pSelf->Stride = 2;
    pSelf->Size = ValuesBufferSize;
    pSelf->Pause = false;
    pSelf->OutValues = pSelf->Values;
    apNode->out_pipe.samples = NULL;
    memset(pSelf->Values, 0, sizeof(pSelf->Values));
    return UMUGU_SUCCESS;
}

static int Process(umugu_node *apNode) {
    if (apNode->out_pipe.samples) {
        return UMUGU_NOOP;
    }

    auto pCtx = umugu_get_context();
    Inspector *pSelf = (Inspector *)apNode;
    auto pInputNode = pCtx->pipeline.nodes[apNode->prev_node];
    if (!pInputNode->out_pipe.samples) {
        umugu_node_dispatch(apNode, UMUGU_FN_PROCESS);
        assert(pInputNode->out_pipe.samples);
    }

    if (pSelf->Pause) {
        return UMUGU_SUCCESS;
    }

    float *pInputFloat = (float *)pInputNode->out_pipe.samples;
    const int Count = pInputNode->out_pipe.count;
    for (int i = pSelf->Offset; i < Count * 2; i += pSelf->Stride) {
        pSelf->Values[pSelf->It] = pInputFloat[i];
        pSelf->Values[pSelf->It + pSelf->Size] = pInputFloat[i];
        pSelf->It++;
        if (pSelf->It >= pSelf->Size) {
            pSelf->It -= pSelf->Size;
        }
    }
    pSelf->OutValues = pSelf->Values + pSelf->It;
    apNode->out_pipe = pInputNode->out_pipe;
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
