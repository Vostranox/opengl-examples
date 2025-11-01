#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <GL/glew.h>

#include "Graphics/Utility.hpp"

class Texture {
  public:
    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(std::uint32_t index, GLenum wrap, GLenum minFilter, GLenum magFilter, const std::string& filename, bool flip = 1) {
        auto [data, width, height, channels] = LoadTexture(filename, flip);
        std::unique_ptr<unsigned char, decltype(&FreeTexture)> pixels(data, FreeTexture);

        GL_CHECK(glGenTextures(1, &id));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, id));

        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter));

        GLenum format{};
        switch (channels) {
        case 1:
            {
                format = GL_RED;
                break;
            }
        case 2:
            {
                format = GL_RG;
                break;
            }
        case 3:
            {
                format = GL_RGB;
                break;
            }
        case 4:
            {
                format = GL_RGBA;
                break;
            }
        }

        internalFormat = pixelFormat = format;
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, pixelFormat, pixelType, pixels.get()));
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));

        this->index = index;
    }

    Texture(std::uint32_t index, GLenum wrap, GLenum minFilter, GLenum magFilter, GLenum format, std::int32_t width,
            std::int32_t height) {
        internalFormat = format;
        pixelFormat    = format;
        if (format == GL_DEPTH_COMPONENT) {
            pixelType = GL_FLOAT;
        } else if (format == GL_DEPTH_STENCIL || format == GL_DEPTH24_STENCIL8) {
            pixelFormat = GL_DEPTH_STENCIL;
            pixelType   = GL_UNSIGNED_INT_24_8;
        }

        GL_CHECK(glGenTextures(1, &id));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, id));

        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter));

        GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, pixelFormat, pixelType, nullptr));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));

        this->index = index;
    }

    void Bind() {
        glActiveTexture(GL_TEXTURE0 + index);
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, id));
    }

    void Unbind() { GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0)); }

    void OnChangeSize(std::uint32_t width, std::uint32_t height) {
        glBindTexture(GL_TEXTURE_2D, id);
        GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, pixelFormat, pixelType, nullptr));
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    ~Texture() { glDeleteTextures(1, &id); }

    std::uint32_t GetID() { return id; }
    operator std::uint32_t() { return id; }

  private:
    std::uint32_t id = 0;
    std::uint32_t index;
    GLenum internalFormat = GL_RGBA;
    GLenum pixelFormat    = GL_RGBA;
    GLenum pixelType      = GL_UNSIGNED_BYTE;
};
