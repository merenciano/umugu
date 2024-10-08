#include "PipelineBuilder.h"
#include "Utilities.h"

#include <imgui/imgui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <umugu/umugu.h>

namespace {
struct Inspector {
    umugu_node node;
    float* out_values;
    int32_t it;
    int32_t padding;
    float* values;
    int32_t size;
    int32_t stride;
    int32_t offset;
    int32_t pause;
};
}

namespace umumk {
void PipelineBuilder::Draw()
{
    auto pCtx = umugu_get_context();

    auto AddNode = [pCtx, this](const auto& It) {
        if (ImGui::CollapsingHeader("Add node")) {
            for (int i = 0; i < pCtx->nodes_info_next; ++i) {
                if (ImGui::Button(pCtx->nodes_info[i].name.str)) {
                    auto NewIt = mNodes.insert(It, (umugu_node*)calloc(1, pCtx->nodes_info[i].size_bytes));
                    memcpy((*NewIt)->name.str, pCtx->nodes_info[i].name.str, UMUGU_NAME_LEN);
#if 0
                    if (!strcmp((*NewIt)->name.str, "Inspector")) {
                        Inspector* Insp = (Inspector*)(*NewIt);
                        Insp->it = 0;
                        Insp->offset = 0;
                        Insp->stride = 2;
                        Insp->size = 2048;
                        Insp->pause = false;
                        Insp->values = (float*)malloc(Insp->size * 2 * sizeof(float));
                        Insp->out_values = Insp->values;
                    }
#endif
                }
            }
        }
    };

    ImGui::Begin("Pipeline builder");

    for (auto It = mNodes.begin(); It != mNodes.end(); ++It) {
        ImGui::PushID(*It);
        AddNode(It);
        DrawNodeWidgets(*It);
        ImGui::PopID();
    }
    AddNode(mNodes.end());

    char Buffer[1024] = { 0 };
    ImGui::InputText("Filename", Buffer, 1024);
    if (ImGui::Button("Export")) {

        FILE* File = fopen(Buffer, "wb");
        if (!File) {
            printf("Could not open the file %s for writing.\n", Buffer);
            return;
        }

        fwrite("IEE", 4, 1, File);

        int Version = 1 << 10;
        fwrite(&Version, 4, 1, File);

        size_t PipelineSize = 0;
        for (auto pNode : mNodes) {
            const umugu_node_info* pInfo = umugu_node_info_find(&pNode->name);
            if (!pInfo) {
                printf("Node info not found.\n");
                return;
            }
            PipelineSize += pInfo->size_bytes;
        }

        fwrite(&PipelineSize, 8, 1, File);

        for (auto pNode : mNodes) {
            const umugu_node_info* pInfo = umugu_node_info_find(&pNode->name);
            if (!pInfo) {
                printf("Node info not found.\n");
                return;
            }

            fwrite(pNode, pInfo->size_bytes, 1, File);
        }
        fwrite("AUU", 4, 1, File);

        fclose(File);
    }

    if (ImGui::Button("Clear")) {
        for (auto pNode : mNodes) {
            free(pNode);
        }
        mNodes.clear();
    }

    ImGui::End();
}
}