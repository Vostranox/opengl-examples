#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <unordered_map>

// A shader file holds every stage of a program; each stage is compiled with VERTEX_SHADER,
// FRAGMENT_SHADER or COMPUTE_SHADER defined, plus any extra defines given for variants.
class Shader {
  public:
    explicit Shader(const std::string& name,
                    std::initializer_list<GLenum> stages       = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER },
                    std::initializer_list<const char*> defines = {});
    ~Shader();
    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;

    void Create(const std::string& name,
                std::initializer_list<GLenum> stages       = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER },
                std::initializer_list<const char*> defines = {});
    void Bind() const;
    void Unbind() const;
    void BindUniformBlock(const std::string& name, unsigned int binding) const;
    operator std::uint32_t() const { return id; }

    template <typename T>
    void Set(const std::string& name, const T& value) const {
        if constexpr (std::is_same_v<T, bool>) {
            Set(name, static_cast<int>(value));
        } else if constexpr (std::is_same_v<T, int>) {
            SetUniform(name, &value, GL_INT);
        } else if constexpr (std::is_same_v<T, float>) {
            SetUniform(name, &value, GL_FLOAT);
        } else if constexpr (std::is_same_v<T, glm::ivec2>) {
            SetUniform(name, &value[0], GL_INT_VEC2);
        } else if constexpr (std::is_same_v<T, glm::vec2>) {
            SetUniform(name, &value[0], GL_FLOAT_VEC2);
        } else if constexpr (std::is_same_v<T, glm::vec3>) {
            SetUniform(name, &value[0], GL_FLOAT_VEC3);
        } else if constexpr (std::is_same_v<T, glm::vec4>) {
            SetUniform(name, &value[0], GL_FLOAT_VEC4);
        } else if constexpr (std::is_same_v<T, glm::mat2>) {
            SetUniform(name, &value[0][0], GL_FLOAT_MAT2);
        } else if constexpr (std::is_same_v<T, glm::mat3>) {
            SetUniform(name, &value[0][0], GL_FLOAT_MAT3);
        } else if constexpr (std::is_same_v<T, glm::mat4>) {
            SetUniform(name, &value[0][0], GL_FLOAT_MAT4);
        } else {
            static_assert(std::is_same_v<T, void>, "Unsupported Shader::Set value type");
        }
    }

  private:
    struct Uniform {
        GLenum type    = 0;
        GLint location = -1;
    };

    void Reflect();
    void SetUniform(const std::string& name, const void* value, GLenum type) const;

    GLuint id = 0;
    std::unordered_map<std::string, Uniform> uniforms;
};
