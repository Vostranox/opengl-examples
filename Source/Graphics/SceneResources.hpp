#pragma once

#include <GL/glew.h>

#include <vector>

class SceneResources {
  public:
    SceneResources()                                 = default;
    SceneResources(const SceneResources&)            = delete;
    SceneResources& operator=(const SceneResources&) = delete;

    ~SceneResources() { Clear(); }

    GLuint Buffer() {
        GLuint id;
        glGenBuffers(1, &id);
        buffers.push_back(id);
        return id;
    }

    GLuint Texture() {
        GLuint id;
        glGenTextures(1, &id);
        textures.push_back(id);
        return id;
    }

    GLuint Framebuffer() {
        GLuint id;
        glGenFramebuffers(1, &id);
        framebuffers.push_back(id);
        return id;
    }

    GLuint Renderbuffer() {
        GLuint id;
        glGenRenderbuffers(1, &id);
        renderbuffers.push_back(id);
        return id;
    }

    GLuint VertexArray() {
        GLuint id;
        glGenVertexArrays(1, &id);
        vertexArrays.push_back(id);
        return id;
    }

    void Clear() {
        glDeleteBuffers(static_cast<GLsizei>(buffers.size()), buffers.data());
        glDeleteTextures(static_cast<GLsizei>(textures.size()), textures.data());
        glDeleteFramebuffers(static_cast<GLsizei>(framebuffers.size()), framebuffers.data());
        glDeleteRenderbuffers(static_cast<GLsizei>(renderbuffers.size()), renderbuffers.data());
        glDeleteVertexArrays(static_cast<GLsizei>(vertexArrays.size()), vertexArrays.data());

        buffers.clear();
        textures.clear();
        framebuffers.clear();
        renderbuffers.clear();
        vertexArrays.clear();
    }

  private:
    std::vector<GLuint> buffers;
    std::vector<GLuint> textures;
    std::vector<GLuint> framebuffers;
    std::vector<GLuint> renderbuffers;
    std::vector<GLuint> vertexArrays;
};
