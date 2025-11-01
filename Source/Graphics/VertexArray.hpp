#pragma once

#include <cstddef>
#include <cstdint>

#include <GL/glew.h>

#include "Graphics/Utility.hpp"

class VertexArray {
  public:
    VertexArray(const VertexArray&)            = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    VertexArray(std::size_t vertexSize, const void* vertexData, std::size_t indexSize, const void* indexData) {
        GL_CHECK(glGenVertexArrays(1, &id));
        GL_CHECK(glBindVertexArray(id));

        GL_CHECK(glGenBuffers(1, &vbo));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo));
        GL_CHECK(glBufferData(GL_ARRAY_BUFFER, vertexSize, vertexData, GL_STATIC_DRAW));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo));

        if (indexData) {
            GL_CHECK(glGenBuffers(1, &ebo));
            GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
            GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexSize, indexData, GL_STATIC_DRAW));
            GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
        }

        GL_CHECK(glBindVertexArray(0));
    }

    ~VertexArray() {
        if (id)
            glDeleteVertexArrays(1, &id);
        if (vbo)
            glDeleteBuffers(1, &vbo);
        if (ebo)
            glDeleteBuffers(1, &ebo);
    }

    void Bind() { GL_CHECK(glBindVertexArray(id)); }
    void Unbind() { GL_CHECK(glBindVertexArray(0)); }

    void AddAttribute(std::uint32_t index,
                      std::int32_t size,
                      GLenum type,
                      std::int32_t normalized,
                      std::size_t stride,
                      std::size_t pointer) {
        Bind();
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        GL_CHECK(glEnableVertexAttribArray(index));
        GL_CHECK(glVertexAttribPointer(index, size, type, normalized, stride, (void*)pointer));
        Unbind();
    }

  private:
    std::uint32_t id  = 0;
    std::uint32_t vbo = 0;
    std::uint32_t ebo = 0;
};
