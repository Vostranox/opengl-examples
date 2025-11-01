#pragma once

#include <cstdint>

#include <GL/glew.h>

class Framebuffer {
  public:
    Framebuffer(const Framebuffer&)            = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    Framebuffer() {
        glGenFramebuffers(1, &id);
        glBindFramebuffer(GL_FRAMEBUFFER, id);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    ~Framebuffer() { glDeleteFramebuffers(1, &id); }

    void Bind() { glBindFramebuffer(GL_FRAMEBUFFER, id); }
    void Unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

    void BindTexture(std::uint32_t texture, GLenum attachment) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture, 0);
    }

    void BindRenderbuffer(std::uint32_t rbo, GLenum attachment) {
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, rbo);
    }

  private:
    std::uint32_t id = 0;
};
