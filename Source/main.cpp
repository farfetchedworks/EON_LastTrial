#include "mcv_platform.h"
#include "engine.h"

extern "C"
{
    // NVIDIA Optimus: Default dGPU instead of iGPU (Driver: 302+)
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    // AMD: Request dGPU High Performance (Driver: 13.35+)
    _declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {

  PROFILE_SET_NFRAMES(10);
  PROFILE_FRAME_BEGINS();

  Remotery* rmt;
  rmt_CreateGlobalInstance(&rmt);

  CApplication app;

  const json& jConfig = loadJson("config.json");
  int wWidth = jConfig.value("width", 1280);
  int wHeight = jConfig.value("height", 720);
  bool fullscreen = jConfig.value("fullscreen", false);

  if (fullscreen) {
      RECT desktop;
      // Get a handle to the desktop window
      const HWND hDesktop = GetDesktopWindow();
      // Get the size of screen to the variable desktop
      GetWindowRect(hDesktop, &desktop);
      wWidth = desktop.right;
      wHeight = desktop.bottom;
  }

  if (!app.init(hInstance, wWidth, wHeight))
    return -1;


  if (!Render.create(app.getHandle(), fullscreen))
    return -2;

  CEngine::get().init();

  app.run();

  CEngine::get().destroy();
  Render.destroy();

  return 0;
}