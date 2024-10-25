#include "UmuguMaker.h"

#include "../../umugu/src/builtin_nodes.h" // TODO: Waveforms in public api
#include "Utilities.h"

#include <umugu/umugu.h>

#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl2.h>
#include <imgui/imgui.h>
#include <imgui/implot.h>

#define UMUGU_PORTAUDIO19_IMPL
#include <umugu/backends/umugu_portaudio19.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

// TODO: Stride (plotting all the points is causing fps drops)
#define PLOT_WAVE_SHAPE(WS, R, G, B)                                                               \
  ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(R, G, B, 1.0f));                                   \
  ImPlot::PlotLine(kWaveformNames[WS], um__waveform_lut[WS], UMUGU_SAMPLE_RATE);                   \
  ImPlot::PopStyleColor()

namespace umumk {
[[maybe_unused]] const char *const kWaveformNames[] = {"SINE", "SAW", "SQUARE", "TRIANGLE",
                                                       "WHITE_NOISE"};

void UmuguMaker::MainMenu() {
  ImGui::BeginMainMenuBar();
  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("Load pipeline")) {
      mShowLoadWindow = true;
    }
    static bool StreamStarted = false;
    if (ImGui::MenuItem("Start stream", "", StreamStarted, true)) {
      umugu_audio_backend_start_stream();
      StreamStarted = true;
    }
    if (ImGui::MenuItem("Stop stream", "", !StreamStarted, true)) {
      umugu_audio_backend_stop_stream();
      StreamStarted = false;
    }
    ImGui::EndMenu();
  }
  if (ImGui::BeginMenu("Tools")) {
    if (ImGui::MenuItem("Pipeline inspector", "", mShowPipelineWindow, true)) {
      mShowPipelineWindow = !mShowPipelineWindow;
    }
    if (ImGui::MenuItem("Pipeline builder", "", mShowBuilderWindow, true)) {
      mShowBuilderWindow = !mShowBuilderWindow;
    }
    ImGui::EndMenu();
  }
  if (ImGui::BeginMenu("Debug")) {
    if (ImGui::MenuItem("Show metrics", "", mShowMetricsWindow, true)) {
      mShowMetricsWindow = !mShowMetricsWindow;
    }

    if (ImGui::MenuItem("Show waveform viewer", "", mShowWaveformWindow, true)) {
      mShowWaveformWindow = !mShowWaveformWindow;
    }

    if (ImGui::MenuItem("Show ImGui Demo", "", mShowDemoWindow, true)) {
      mShowDemoWindow = !mShowDemoWindow;
    }
    ImGui::EndMenu();
  }

  ImGui::EndMainMenuBar();
}

void UmuguMaker::ToolWindows() {
  if (mShowLoadWindow) {
    static char Buffer[1024] = "../assets/pipelines/";
    ImGui::Begin("Load file");
    ImGui::InputText("Pipeline file to load", Buffer, 1024);
    if (ImGui::Button("Load")) {
      umugu_load_pipeline_bin(Buffer);
      mShowLoadWindow = false;
    }
    ImGui::End();
  }
  if (mShowPipelineWindow) {
    mInspector.Show();
  }
  if (mShowBuilderWindow) {
    mBuilder.Show();
  }

  if (mShowMetricsWindow) {
    ImGui::Begin("Metrics");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::End();
  }

  if (mShowWaveformWindow) {
    ImGui::Begin("Waveform viewer");
    if (ImPlot::BeginPlot("Wave form")) {
      PLOT_WAVE_SHAPE(UM__WAVEFORM_SINE, 1.0f, 0.0f, 0.0f);
      PLOT_WAVE_SHAPE(UM__WAVEFORM_SAW, 0.0f, 1.0f, 0.0f);
      PLOT_WAVE_SHAPE(UM__WAVEFORM_SQUARE, 0.0f, 0.0f, 1.0f);
      PLOT_WAVE_SHAPE(UM__WAVEFORM_TRIANGLE, 1.0f, 0.0f, 1.0f);
      PLOT_WAVE_SHAPE(UM__WAVEFORM_WHITE_NOISE, 1.0f, 1.0f, 0.0f);
      ImPlot::EndPlot();
    }
    ImGui::End();
  }

  if (mShowDemoWindow) {
    ImGui::ShowDemoWindow(&mShowDemoWindow);
  }
}

UmuguMaker::UmuguMaker() {
  umugu_ctx *pCtx = umugu_get_context();
  pCtx->alloc = malloc;
  pCtx->free = free;
  pCtx->io.log = printf;

  umugu_init();
  umugu_audio_backend_init();
  umugu_load_pipeline_bin("../assets/pipelines/default.bin");

  umugu_name LimitName = {"Limiter"};
  umugu_name InspecName = {"Inspector"};
  umugu_name OsciName = {"Oscilloscope"};
  int64_t base_offset =
      umugu_node_info_find(&LimitName)->size_bytes + umugu_node_info_find(&InspecName)->size_bytes;
  int64_t osc_sz = umugu_node_info_find(&OsciName)->size_bytes;

  int32_t KeyMaps[13] = {ImGuiKey_A, ImGuiKey_W, ImGuiKey_S, ImGuiKey_E, ImGuiKey_D,
                         ImGuiKey_F, ImGuiKey_T, ImGuiKey_G, ImGuiKey_Y, ImGuiKey_H,
                         ImGuiKey_U, ImGuiKey_J, ImGuiKey_K};
  for (int i = 0; i < 1; ++i) {
    pCtx->io.in.keys[i].mapped_key = KeyMaps[i];
    pCtx->io.in.keys[i].node_pipeline_offset = base_offset + (osc_sz * i);
    pCtx->io.in.keys[i].var_idx = 0;
    pCtx->io.in.keys[i].value.i = 0;
  }

  umugu_audio_backend_start_stream();
  OpenWindow();
}

UmuguMaker::~UmuguMaker() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(mpGlContext);
  SDL_DestroyWindow((SDL_Window *)mpWindowHandle);
  SDL_Quit();

  umugu_audio_backend_stop_stream();
  umugu_audio_backend_close();
  umugu_close();
}

bool UmuguMaker::Update() {
  bool KeepRunning = PollEvents();

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
  umugu_ctx *pCtx = umugu_get_context();

  constexpr int kValues[] = {32 * 8, 34 * 8, 36 * 8, 38 * 8, 41 * 8, 43 * 8, 46 * 8,
                             49 * 8, 52 * 8, 55 * 8, 58 * 8, 61 * 8, 64 * 8};
  for (int i = 0; i < 1; ++i) {
    if (ImGui::IsKeyDown((ImGuiKey)pCtx->io.in.keys[i].mapped_key)) {
      pCtx->io.in.keys[i].value.i = kValues[i];
      pCtx->io.in.dirty_keys |= (1UL << i);
    } else {
      pCtx->io.in.keys[i].value.i = 0;
      pCtx->io.in.dirty_keys |= (1UL << i);
    }
  }

  MainMenu();

  ToolWindows();

  Render();
  return KeepRunning;
}

void UmuguMaker::Render() {
  glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
  glClearColor(mClearColor[0], mClearColor[1], mClearColor[2], mClearColor[3]);
  glClear(GL_COLOR_BUFFER_BIT);

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  SDL_GL_SwapWindow((SDL_Window *)mpWindowHandle);
}

bool UmuguMaker::PollEvents() {
  SDL_Event Event;
  while (SDL_PollEvent(&Event)) {
    ImGui_ImplSDL2_ProcessEvent(&Event);
    if (Event.type == SDL_QUIT ||
        (Event.type == SDL_WINDOWEVENT && Event.window.event == SDL_WINDOWEVENT_CLOSE &&
         Event.window.windowID == SDL_GetWindowID((SDL_Window *)mpWindowHandle))) {
      return false;
    }
  }
  return true;
}

void UmuguMaker::OpenWindow() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
    printf("Error: %s\n", SDL_GetError());
    return;
  }

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
  int Flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
  mpWindowHandle = SDL_CreateWindow("Venga, a ver si te buscas una musiquilla guapa, no colega?",
                                    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, mWindowWidth,
                                    mWindowHeight, Flags);
  mpGlContext = SDL_GL_CreateContext((SDL_Window *)mpWindowHandle);
  SDL_GL_MakeCurrent((SDL_Window *)mpWindowHandle, mpGlContext);
  SDL_GL_SetSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForOpenGL(mpWindowHandle, mpGlContext);
  ImGui_ImplOpenGL3_Init(mGlslVersion);
}

} // namespace umumk
