#pragma once

#include <cstdint>

#include <GL/glew.h>

class Renderbuffer {
  public:
    Renderbuffer(const Renderbuffer&)            = delete;
    Renderbuffer& operator=(const Renderbuffer&) = delete;

    Renderbuffer(GLenum componentType, std::int32_t width, std::int32_t height) : componentType(componentType) {
        glGenRenderbuffers(1, &id);
        glBindRenderbuffer(GL_RENDERBUFFER, id);
        glRenderbufferStorage(GL_RENDERBUFFER, componentType, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    ~Renderbuffer() { glDeleteRenderbuffers(1, &id); }

    void OnChangeSize(std::uint32_t width, std::uint32_t height) {
        glBindRenderbuffer(GL_RENDERBUFFER, id);
        glRenderbufferStorage(GL_RENDERBUFFER, componentType, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    operator std::uint32_t() { return id; }

  private:
    std::uint32_t id = 0;
    GLenum componentType;
};
