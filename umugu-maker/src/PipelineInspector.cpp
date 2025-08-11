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
              ((umugu_portaudio *)umugu_io().backend_data)->time_margin_sec);
  for (int i = 0; i < umugu_pipeline().node_count; ++i) {
    NodeWidgets(umugu_pipeline().nodes[i]);
  }
  ImGui::End();
}

void PipelineInspector::NodeWidgets(umugu_node *apNode) {
  ImGui::PushID(apNode);
  ImGui::Separator();

  umumk::DrawNodeWidgets(apNode);

  ImGui::PopID();
}
} // namespace umumk
