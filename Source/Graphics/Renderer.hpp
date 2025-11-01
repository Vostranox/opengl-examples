#pragma once

#include <iostream>

#include <GL/glew.h>

#include "Graphics/Utility.hpp"

class Renderer {
  public:
    Renderer() = default;

    void DrawArrays(GLenum type, std::uint32_t first, std::uint32_t number) {
        GL_CHECK(glDrawArrays(type, first, number));
    }

    void SetClearColor(float x, float y, float z, float a) {
        GL_CHECK(glClearColor(x, y, z, a));
    }

    void Clear(GLenum name) {
        GL_CHECK(glClear(name));
    }

    void Enable(GLenum name) {
        glEnable(name);
    }

    void Disable(GLenum name) {
        glDisable(name);
    }
};
