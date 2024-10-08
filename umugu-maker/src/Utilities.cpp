#include "Utilities.h"

#include <imgui/imgui.h>
#include <imgui/implot.h>
#include <stdio.h>
#include <umugu/umugu.h>

namespace umumk {
void DrawNodeWidgets(umugu_node* aNode)
{
    const umugu_node_info* pInfo = umugu_node_info_find(&aNode->name);
    if (!pInfo) {
        printf("Node info not found.\n");
        return;
    }

    ImGui::TextUnformatted(pInfo->name.str);
    const int VarCount = pInfo->var_count;
    for (int i = 0; i < VarCount; ++i) {
        const umugu_var_info* Var = &pInfo->vars[i];
        char* Value = ((char*)aNode + Var->offset_bytes);
        int Step = 1;
        int FastStep = 50;
        switch (Var->type) {
        case UMUGU_TYPE_PLOTLINE: {
            if (!*(float**)Value)
                break;
            if (ImPlot::BeginPlot("PlotLine")) {
                ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                ImPlot::PlotLine(Var->name.str, *(float**)Value, Var->count);
                ImPlot::PopStyleColor();
                ImPlot::EndPlot();
            }
            break;
        }

        case UMUGU_TYPE_FLOAT: {
            ImGui::SliderScalarN(Var->name.str, ImGuiDataType_Float, Value, Var->count, &Var->misc.range.min, &Var->misc.range.max, "%.2f");
            break;
        }

        case UMUGU_TYPE_INT32: {
            ImGui::InputScalarN(Var->name.str, ImGuiDataType_S32, Value, Var->count, &Step, &FastStep, "%d");
            break;
        }

        case UMUGU_TYPE_INT16: {
            ImGui::InputScalarN(Var->name.str, ImGuiDataType_S16, Value, Var->count, &Step, &FastStep, "%d");
            break;
        }

        case UMUGU_TYPE_UINT8: {
            ImGui::InputScalarN(Var->name.str, ImGuiDataType_U8, Value, Var->count, &Step, &FastStep, "%u");
            break;
        }

        case UMUGU_TYPE_BOOL: {
            ImGui::Checkbox(Var->name.str, (bool*)Value);
            break;
        }

        case UMUGU_TYPE_TEXT: {
            ImGui::InputText(Var->name.str, Value, Var->count);
            break;
        }

        default: {
            ImGui::Text("Type %d not implemented", Var->type);
            break;
        }
        }
    }
}
}