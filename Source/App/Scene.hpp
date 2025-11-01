#pragma once

#include "App/Overlay.hpp"
#include "App/Window.hpp"

class SceneContext {
  public:
    SceneContext(Window& window, Overlay& overlay, const char* name = nullptr);

    bool BeginFrame();
    void EndFrame();

    Window& window;
    Overlay& overlay;

  private:
    const char* name;
    bool back = false;
};

void ResetGraphicsState(Window& window);
