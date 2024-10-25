#include "PipelineInspector.h"

#include "Utilities.h"

#include <umugu/umugu.h>

#include <imgui/imgui.h>

#include <stdio.h>

namespace umumk {
void PipelineInspector::Show() {
  ImGui::Begin("Pipeline graph");
  ImGui::Text("Available time for the callback: %lf", umugu_get_context()->io.time_margin_sec);
  for (int i = 0; i < umugu_get_context()->pipeline.node_count; ++i) {
    NodeWidgets(umugu_get_context()->pipeline.nodes[i]);
  }
  ImGui::End();
}

void PipelineInspector::NodeWidgets(umugu_node *apNode) {
  const umugu_node_info *pInfo = umugu_node_info_find(&apNode->name);
  if (!pInfo) {
    printf("Node info not found.\n");
    return;
  }

  ImGui::PushID(apNode);
  ImGui::Separator();

  umumk::DrawNodeWidgets(apNode);

  ImGui::PopID();
}
} // namespace umumk
