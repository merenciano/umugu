#ifndef __UMUGU_MAKER_H__
#define __UMUGU_MAKER_H__

#include "PipelineBuilder.h"
#include "PipelineInspector.h"

namespace umumk {
class UmuguMaker {
public:
  UmuguMaker();
  ~UmuguMaker();

  bool Update();

private:
  bool PollEvents();
  void Render();

  PipelineInspector mInspector;
  PipelineBuilder mBuilder;

  void OpenWindow();
  void *mpWindowHandle = NULL;
  const char *mWindowTitle;
  int mWindowWidth = 1600;
  int mWindowHeight = 900;
  const char *mGlslVersion = "#version 130";
  void *mpGlContext = NULL;
  float mClearColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};

  void MainMenu();
  void ToolWindows();
  void MetricsWindow();
  void PipelineWindow();
  void BuilderWindow();
  bool mShowPipelineWindow = true;
  bool mShowDemoWindow = false;
  bool mShowMetricsWindow = false;
  bool mShowWaveformWindow = false;
  bool mShowBuilderWindow = false;
};
} // namespace umumk

#endif
