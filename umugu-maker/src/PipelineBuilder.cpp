#include "PipelineBuilder.h"

#include "../../umugu/src/umugu_internal.h"
#include "Utilities.h"

#include <umugu/umugu.h>

#include <imgui/imgui.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace umumk {
void PipelineBuilder::Show() {
  auto pCtx = umugu_get();
  auto AddNode = [pCtx, this](const auto &It) {
    if (ImGui::CollapsingHeader("Add node")) {
      for (int i = 0; i < pCtx->nodes_info_next; ++i) {
        if (ImGui::Button(pCtx->nodes_info[i].name.str)) {
          auto NewIt = mNodes.insert(It, (umugu_node *)calloc(1, pCtx->nodes_info[i].size_bytes));
          (*NewIt)->info_idx = i;
        }
      }
    }
  };

  ImGui::Begin("Pipeline builder");
  static umugu_name NodeName;
  if (ImGui::Button("Load info")) {
    um_node_info_load(&NodeName);
    memset(NodeName.str, 0, 32);
  }
  ImGui::SameLine();
  ImGui::InputText("Node name", NodeName.str, UMUGU_NAME_LEN);

  for (auto It = mNodes.begin(); It != mNodes.end(); ++It) {
    ImGui::PushID(*It);
    AddNode(It);
    DrawNodeWidgets(*It);
    ImGui::PopID();
  }
  AddNode(mNodes.end());

  static char Buffer[1024] = "../assets/pipelines/";
  ImGui::InputText("Filename", Buffer, 1024);
  if (ImGui::Button("Export")) {
    FILE *File = fopen(Buffer, "wb");
    if (!File) {
      printf("Could not open the file %s for writing.\n", Buffer);
      return;
    }

    fwrite("IEE", 4, 1, File);

    int Version = 3 << 10;
    fwrite(&Version, 4, 1, File);
    size_t PipelineSize = 0;
    for (auto pNode : mNodes) {
      const umugu_node_info *pInfo = &pCtx->nodes_info[pNode->info_idx];
      if (!pInfo) {
        printf("Node info not found.\n");
        return;
      }
      PipelineSize += pInfo->size_bytes;
    }

    fwrite(&PipelineSize, 8, 1, File);

    for (auto pNode : mNodes) {
      const umugu_node_info *pInfo = &pCtx->nodes_info[pNode->info_idx];
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
} // namespace umumk
