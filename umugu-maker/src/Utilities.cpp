#include "Utilities.h"

#include <umugu/umugu.h>

#include <imgui/imgui.h>
#include <imgui/implot.h>

#include <stdio.h>

namespace umumk {
void DrawNodeWidgets(umugu_node *aNode) {
  const umugu_node_info &Info = umugu_get_context()->nodes_info[aNode->info_idx];

  ImGui::TextUnformatted(Info.name.str);
  const int VarCount = Info.var_count;
  for (int i = 0; i < VarCount; ++i) {
    const umugu_var_info &Var = Info.vars[i];
    char *Value = ((char *)aNode + Var.offset_bytes);
    int Step = 1;
    int FastStep = 50;
    switch (Var.type) {
    case UMUGU_TYPE_FLOAT: {
      if (Var.flags & UMUGU_VAR_PLOTLINE) {
        if (!*(float **)Value)
          break;
        if (ImPlot::BeginPlot("PlotLine")) {
          ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
          ImPlot::PlotLine(Var.name.str, *(float **)Value, Var.count);
          ImPlot::PopStyleColor();
          ImPlot::EndPlot();
        }
      } else {
        ImGui::SliderScalarN(Var.name.str, ImGuiDataType_Float, Value, Var.count,
                             &Var.misc.rangef.min, &Var.misc.rangef.max, "%.2f");
      }
      break;
    }

    case UMUGU_TYPE_INT32: {
      ImGui::InputScalarN(Var.name.str, ImGuiDataType_S32, Value, Var.count, &Step, &FastStep,
                          "%d");
      break;
    }

    case UMUGU_TYPE_INT16: {
      ImGui::InputScalarN(Var.name.str, ImGuiDataType_S16, Value, Var.count, &Step, &FastStep,
                          "%d");
      break;
    }

    case UMUGU_TYPE_UINT8: {
      ImGui::InputScalarN(Var.name.str, ImGuiDataType_U8, Value, Var.count, &Step, &FastStep, "%u");
      break;
    }

    case UMUGU_TYPE_BOOL: {
      ImGui::Checkbox(Var.name.str, (bool *)Value);
      break;
    }

    case UMUGU_TYPE_TEXT: {
      ImGui::InputText(Var.name.str, Value, Var.count);
      break;
    }

    default: {
      ImGui::Text("Type %d not implemented", Var.type);
      break;
    }
    }
  }
}
} // namespace umumk
