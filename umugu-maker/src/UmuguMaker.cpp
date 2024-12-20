#include "UmuguMaker.h"

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
  ImPlot::PlotLine(kWaveformNames[WS], umugu_waveform_lut[WS], UMUGU_SAMPLE_RATE);                 \
  ImPlot::PopStyleColor()

namespace umumk {
[[maybe_unused]] const char *const kWaveformNames[] = {"SINE",     "SAWSIN", "SAW",
                                                       "TRIANGLE", "SQUARE", "WHITE_NOISE"};

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
      umugu_import_pipeline(Buffer);
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
#if 0
    ImGui::Begin("Waveform viewer");
    if (ImPlot::BeginPlot("Wave form")) {
      ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
      ImPlot::PlotLine(kWaveformNames[UMUGU_WAVE_SINE], umugu_waveform_lut[WS], UMUGU_SAMPLE_RATE);
      ImPlot::PopStyleColor();
      PLOT_WAVE_SHAPE(UMUGU_WAVE_SINE, 1.0f, 0.0f, 0.0f);
      PLOT_WAVE_SHAPE(UMUGU_WAVE_SAW, 0.0f, 1.0f, 0.0f);
      PLOT_WAVE_SHAPE(UMUGU_WAVE_SQUARE, 0.0f, 0.0f, 1.0f);
      PLOT_WAVE_SHAPE(UMUGU_WAVE_TRIANGLE, 1.0f, 0.0f, 1.0f);
      PLOT_WAVE_SHAPE(UMUGU_WAVE_WHITE_NOISE, 1.0f, 1.0f, 0.0f);
      ImPlot::EndPlot();
    }
    ImGui::End();
#endif
  }

  if (mShowDemoWindow) {
    ImGui::ShowDemoWindow(&mShowDemoWindow);
  }
}

static void ErrorFatal(int err, const char *msg, const char *file, int line) {
  printf("Umugu fatal error in %s (%d): Code %d - %s.\n", file, line, err, msg);
  __builtin_trap();
}

static size_t CopyFileToBuffer(const char *filename, void *buffer, size_t buf_size) {
  FILE *fp = fopen(filename, "r");
  fseek(fp, 0, SEEK_END);
  size_t size = ftell(fp);
  if ((size + 1) > buf_size) {
    fclose(fp);
    return size + 1;
  }

  fseek(fp, 0, SEEK_SET);
  fread(buffer, 1, size, fp);
  ((char *)buffer)[size] = '\0';
  fclose(fp);
  return size + 1;
}

UmuguMaker::UmuguMaker() {
  constexpr size_t Size = 1024 * 1024 * 1024;
  umugu_config Config = {.log_fn = printf,
                         .fatal_err_fn = ErrorFatal,
                         .load_file = CopyFileToBuffer,
                         .config_file = "../assets/config.ucg",
                         .arena = malloc(Size),
                         .arena_size = Size,
                         .fallback_ppln = {},
                         .fallback_ppln_node_count = 0};
  auto *pCtx = umugu_load(&Config);

  pCtx->io.backend.channel_count = 2;
  pCtx->io.backend.sample_rate = 48000;
  pCtx->io.backend.format = UMUGU_TYPE_FLOAT;
  pCtx->io.backend.interleaved_channels = true;
  pCtx->io.backend.audio_input = false;
  pCtx->io.backend.audio_output = true;
  pCtx->io.backend.audio_callback = umugu_process;
  if (pCtx->state != UMUGU_STATE_IDLE) {
    pCtx->io.log("Something went wrong initializing umugu.\n");
  }

  umugu_audio_backend_init();
  // umugu_audio_backend_start_stream();
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
}

bool UmuguMaker::Update() {
  bool KeepRunning = PollEvents();

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

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
