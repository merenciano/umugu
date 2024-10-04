#include "umugu_editor.h"

#include "umugu.h"
#include "umugu_internal.h"

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
static bool show_demo_window = true;

namespace umugu_editor {
struct Window {
    void* native_win;
    const char* title;
    int width, height;
    const char* glsl_ver;
    void* gl_context;
};

Window window;

const char* const SHAPE_NAMES[] = {
    "SINE",
    "SAW",
    "SQUARE",
    "TRIANGLE",
    "WHITE_NOISE"
};

static void DrawUnitUI(umugu_node** node)
{
#if 0
    switch ((*node)->type) {
    case UMUGU_NT_OSCOPE: {
        auto self = (umugu_node_oscope*)*node;
        *node = (umugu_node*)((char*)*node + sizeof(umugu_node_oscope));
        ImGui::PushID(self);
        ImGui::Separator();
        ImGui::Text("Osciloscope");
        if (ImGui::BeginCombo("Wave Shape", SHAPE_NAMES[self->shape], 0)) {
            for (int j = 0; j < UMUGU_WF_COUNT; ++j) {
                const bool selected = (self->shape == (umugu_waveforms)j);
                if (ImGui::Selectable(SHAPE_NAMES[j], selected)) {
                    self->shape = (umugu_waveforms)j;
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::InputInt("Frequency", &self->freq, 1, 10, 0);
        ImGui::PopID();
        break;
    }

    case UMUGU_NT_WAV_PLAYER: {
        um__wav_player* self = (um__wav_player*)*node;
        *node = (umugu_node*)((char*)*node + sizeof(um__wav_player));
        ImGui::PushID(self);
        ImGui::Separator();
        ImGui::Text("File: %s", self->filename);
        ImGui::PopID();
        break;
    }

    case UMUGU_NT_MIXER: {
        ImGui::PushID(*node);
        *node = (umugu_node*)((char*)*node + sizeof(um__mixer));
        ImGui::Separator();
        ImGui::Text("Mix");
        ImGui::PopID();
        DrawUnitUI(node);
        break;
    }

    case UMUGU_NT_INSPECTOR: {
        umugu_node_inspector* self = (umugu_node_inspector*)*node;
        *node = (umugu_node*)((char*)*node + sizeof(umugu_node_inspector));
        ImGui::PushID(self);
        ImGui::Separator();
        if (ImPlot::BeginPlot("Inspector")) {
            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            ImPlot::PlotLine("Wave", self->values + self->it, self->size);
            ImPlot::PopStyleColor();

            ImPlot::EndPlot();
        }
        if (self->pause) {
            ImGui::InputInt("Stride", &self->stride, 0, 1024, 0);
            ImGui::InputInt("Offset", &self->offset, 0, 1024, 0);
        }
        ImGui::Checkbox("Paused", (bool*)&self->pause);
        ImGui::PopID();
        DrawUnitUI(node);
        break;
    }

    case UMUGU_NT_AMPLITUDE: {
        um__amplitude* self = (um__amplitude*)*node;
        *node = (umugu_node*)((char*)*node + sizeof(um__amplitude));
        ImGui::PushID(self);
        ImGui::Separator();
        ImGui::Text("Volume");
        ImGui::InputFloat("Multiplier:", &self->multiplier, 0, 100, 0);
        ImGui::PopID();
        DrawUnitUI(node);
        break;
    }

    case UMUGU_NT_LIMITER: {
        um__limiter* self = (um__limiter*)*node;
        *node = (umugu_node*)((char*)*node + sizeof(um__limiter));
        ImGui::PushID(self);
        ImGui::Separator();
        ImGui::Text("Clamp");
        ImGui::InputFloat("Lower Limit:", &self->min, -2.0f, 2.0f, 0);
        ImGui::InputFloat("Upper Limit:", &self->max, -2.0f, 2.0f, 0);
        ImGui::PopID();
        DrawUnitUI(node);
        break;
    }

    default:
        break;
    }
#endif
}

static void GraphWindow()
{
    umugu_node* it = umugu_get_context()->pipeline.root;
    ImGui::Begin("Graph");
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
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    window.native_win = SDL_CreateWindow("Venga, a ver si te buscas una musiquilla guapa, no colega?", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, window_flags);
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

    if (show_demo_window) {
        ImGui::ShowDemoWindow(&show_demo_window);
    }

    GraphWindow();

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
    umugu_load_pipeline_bin("../assets/pipelines/littlewing.bin");

    umugu_start_stream();
    InitUI();
}
} // namespace umugu_editor