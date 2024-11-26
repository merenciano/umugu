#include "../umugu/src/umugu_internal.h"

#include <umugu/umugu.h>

#include <string.h>

constexpr int ValuesBufferSize = 2048;

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

static const umugu_attrib_info metadata[] = {
    {.name = {.str = "Values"},
     .offset_bytes = offsetof(Inspector, OutValues),
     .type = UMUGU_TYPE_FLOAT,
     .count = 2048,
     .flags = UMUGU_ATTR_RDONLY | UMUGU_ATTR_PLOTLINE,
     .misc = {.rangei = {.min = 0, .max = 2048}}},
    {.name = {.str = "Stride"},
     .offset_bytes = offsetof(Inspector, Stride),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .flags = UMUGU_ATTR_FLAG_NONE,
     .misc = {.rangei = {.min = 1, .max = 1024}}},
    {.name = {.str = "Offset"},
     .offset_bytes = offsetof(Inspector, Offset),
     .type = UMUGU_TYPE_INT32,
     .count = 1,
     .flags = UMUGU_ATTR_FLAG_NONE,
     .misc = {.rangei = {.min = 1, .max = 1024}}},
    {.name = {.str = "Pause"},
     .offset_bytes = offsetof(Inspector, Pause),
     .type = UMUGU_TYPE_BOOL,
     .count = 1,
     .flags = UMUGU_ATTR_FLAG_NONE,
     .misc = {.rangei = {.min = 0, .max = 1}}},
};

extern "C" const int size = (int)sizeof(Inspector);
extern "C" umugu_node_func getfn(int fn);
extern "C" const umugu_attrib_info *attribs;
extern "C" const int attrib_count = sizeof(metadata) / sizeof(*metadata);

const umugu_attrib_info *attribs = &metadata[0];

static int Init(umugu_ctx *apCtx, umugu_node *apNode, umugu_fn_flags aFlags) {
    UM_UNUSED(apCtx);
    Inspector *pSelf = (Inspector *)apNode;
    if (aFlags & UMUGU_FN_INIT_DEFAULTS) {
        pSelf->It = 0;
        pSelf->Offset = 0;
        pSelf->Stride = 2;
        pSelf->Pause = false;
    }
    pSelf->Size = ValuesBufferSize;
    pSelf->OutValues = pSelf->Values;
    apNode->out_pipe.samples = NULL;
    memset(pSelf->Values, 0, sizeof(pSelf->Values));
    return UMUGU_SUCCESS;
}

static int Process(umugu_ctx *apCtx, umugu_node *apNode,
                   umugu_fn_flags aFlags) {
    UM_UNUSED(aFlags);
    Inspector *pSelf = (Inspector *)apNode;
    auto pInputNode = apCtx->pipeline.nodes[apNode->prev_node];

    if (pSelf->Pause) {
        return UMUGU_SUCCESS;
    }

    float *pInputFloat = (float *)pInputNode->out_pipe.samples;
    const int Count = pInputNode->out_pipe.frame_count;
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

umugu_node_func getfn(int32_t fn) {
    switch (fn) {
        case UMUGU_FN_INIT:
            return Init;
        case UMUGU_FN_PROCESS:
            return Process;
        default:
            return nullptr;
    }
}
