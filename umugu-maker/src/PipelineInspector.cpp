#include "PipelineInspector.h"

#include "Utilities.h"

#include <umugu/umugu.h>

#include <imgui/imgui.h>

#include <stdio.h>

namespace umumk {
void PipelineInspector::Show() {
  umugu_node *It = umugu_get_context()->pipeline.root;
  ImGui::Begin("Pipeline graph");
  ImGui::Text("Available time for the callback: %lf", umugu_get_context()->io.time_margin_sec);
  if (umugu_get_context()->pipeline.size_bytes) {
    NodeWidgets(&It);
  }
  ImGui::End();
}

void PipelineInspector::NodeWidgets(umugu_node **apNode) {
  umugu_node *pSelf = *apNode;
  const umugu_node_info *pInfo = umugu_node_info_find(&pSelf->name);
  if (!pInfo) {
    printf("Node info not found.\n");
    return;
  }
  *apNode = (umugu_node *)((char *)*apNode + pInfo->size_bytes);

  umugu_ctx *pCtx = umugu_get_context();
  if (pCtx->pipeline.size_bytes > ((char *)*apNode - (char *)pCtx->pipeline.root)) {
    NodeWidgets(apNode);
  }

  ImGui::PushID(pSelf);
  ImGui::Separator();

  umumk::DrawNodeWidgets(pSelf);

  ImGui::PopID();
}
} // namespace umumk
