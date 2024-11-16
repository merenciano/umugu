#include "PipelineInspector.h"

#include "Utilities.h"

#include <umugu/backends/umugu_portaudio19.h>
#include <umugu/umugu.h>

#include <imgui/imgui.h>

#include <stdio.h>

namespace umumk {
void PipelineInspector::Show() {
  ImGui::Begin("Pipeline graph");
  ImGui::Text("Available time for the callback: %lf",
              ((umugu_portaudio *)(umugu_get_context()->io.backend_data))->time_margin_sec);
  for (int i = 0; i < umugu_get_context()->pipeline.node_count; ++i) {
    NodeWidgets(umugu_get_context()->pipeline.nodes[i]);
  }
  ImGui::End();
}

void PipelineInspector::NodeWidgets(umugu_node *apNode) {
  const umugu_node_info *pInfo = &umugu_get_context()->nodes_info[apNode->type];
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
