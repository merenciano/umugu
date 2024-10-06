#include "umugu_editor.h"

#include "../../umugu/src/nodes/builtin_nodes.h" // TODO: Waveforms in public api
#include <umugu/umugu.h>

#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/imgui.h"
#include "imgui/implot.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

// TODO: Stride (plotting all the points is causing fps drops)
#define PLOT_WAVE_SHAPE(WS, R, G, B)                                            \
    ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(R, G, B, 1.0f));              \
    ImPlot::PlotLine(SHAPE_NAMES[WS], um__waveform_lut[WS], UMUGU_SAMPLE_RATE); \
    ImPlot::PopStyleColor()

static ImGuiIO* io;

namespace umugu_editor {
struct Window {
    void* native_win;
    const char* title;
    int width, height;
    const char* glsl_ver;
    void* gl_context;
};

Window window;

[[maybe_unused]] const char* const SHAPE_NAMES[] = {
    "SINE",
    "SAW",
    "SQUARE",
    "TRIANGLE",
    "WHITE_NOISE"
};

static void DrawUnitUI(umugu_node** node)
{
    umugu_node* self = *node;
    const umugu_node_info* info = umugu_node_info_find(&self->name);
    if (!info) {
        printf("Node info not found.\n");
        return;
    }
    *node = (umugu_node*)((char*)*node + info->size_bytes);

    umugu_ctx* ctx = umugu_get_context();
    if (ctx->pipeline.size_bytes > ((char*)*node - (char*)ctx->pipeline.root)) {
        DrawUnitUI(node);
    }

    ImGui::PushID(self);
    ImGui::Separator();
    ImGui::TextUnformatted(info->name.str);
    const int var_count = info->var_count;
    for (int i = 0; i < var_count; ++i) {
        const umugu_var_info* var = &info->vars[i];
        char* value = ((char*)self + var->offset_bytes);
        int step = 1;
        int fastep = 50;
        switch (var->type) {
        case UMUGU_TYPE_PLOTLINE: {
            if (ImPlot::BeginPlot("PlotLine")) {
                ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                ImPlot::PlotLine(var->name.str, *(float**)value, var->count);
                ImPlot::PopStyleColor();
                ImPlot::EndPlot();
            }
            break;
        }

        case UMUGU_TYPE_FLOAT: {
            ImGui::SliderScalarN(var->name.str, ImGuiDataType_Float, value, var->count, &var->misc.range.min, &var->misc.range.max, "%.2f");
            break;
        }

        case UMUGU_TYPE_INT32: {
            ImGui::InputScalarN(var->name.str, ImGuiDataType_S32, value, var->count, &step, &fastep, "%d");
            break;
        }

        case UMUGU_TYPE_INT16: {
            ImGui::InputScalarN(var->name.str, ImGuiDataType_S16, value, var->count, &step, &fastep, "%d");
            break;
        }

        case UMUGU_TYPE_UINT8: {
            ImGui::InputScalarN(var->name.str, ImGuiDataType_U8, value, var->count, &step, &fastep, "%u");
            break;
        }

        case UMUGU_TYPE_BOOL: {
            ImGui::Checkbox(var->name.str, (bool*)value);
            break;
        }

        case UMUGU_TYPE_TEXT: {
            ImGui::InputText(var->name.str, value, var->count);
            break;
        }

        default: {
            ImGui::Text("Type %d not implemented", var->type);
            break;
        }
        }
    }

    ImGui::PopID();
}

static void GraphWindow()
{
    umugu_node* it = umugu_get_context()->pipeline.root;
    ImGui::Begin("Graph");
    ImGui::Text("Available time for the callback: %lf", umugu_get_context()->audio_output.time_margin_sec);
    DrawUnitUI(&it);
    ImGui::End();
}

static void InitUI()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return;
    }

    window.glsl_ver = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    int window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    window.native_win = SDL_CreateWindow("Venga, a ver si te buscas una musiquilla guapa, no colega?", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 720, window_flags);
    window.gl_context = SDL_GL_CreateContext((SDL_Window*)window.native_win);
    SDL_GL_MakeCurrent((SDL_Window*)window.native_win, window.gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    io = &ImGui::GetIO();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window.native_win, window.gl_context);
    ImGui_ImplOpenGL3_Init(window.glsl_ver);
}

bool PollEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID((SDL_Window*)window.native_win))) {
            return true;
        }
    }
    return false;
}

void Render()
{
    glViewport(0, 0, (int)io->DisplaySize.x, (int)io->DisplaySize.y);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    GraphWindow();

    static bool show_demo_window = false;
    if (show_demo_window) {
        ImGui::ShowDemoWindow(&show_demo_window);
    }

    ImGui::Begin("Hello, world!");
    ImGui::Checkbox("Demo Window", &show_demo_window);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);
    if (ImPlot::BeginPlot("Wave form")) {
        PLOT_WAVE_SHAPE(UM__WAVEFORM_SINE, 1.0f, 0.0f, 0.0f);
        PLOT_WAVE_SHAPE(UM__WAVEFORM_SAW, 0.0f, 1.0f, 0.0f);
        PLOT_WAVE_SHAPE(UM__WAVEFORM_SQUARE, 0.0f, 0.0f, 1.0f);
        PLOT_WAVE_SHAPE(UM__WAVEFORM_TRIANGLE, 1.0f, 0.0f, 1.0f);
        PLOT_WAVE_SHAPE(UM__WAVEFORM_WHITE_NOISE, 1.0f, 1.0f, 0.0f);
        ImPlot::EndPlot();
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow((SDL_Window*)window.native_win);
}

void Close()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(window.gl_context);
    SDL_DestroyWindow((SDL_Window*)window.native_win);
    SDL_Quit();

    umugu_close();
}

void Init()
{
    umugu_init();
    umugu_load_pipeline_bin("../assets/pipelines/plugtest.bin");

    umugu_start_stream();
    InitUI();
}
} // namespace umugu_editor
