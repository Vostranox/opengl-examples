#include "Graphics/Shader.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace {
    const char* StageDefine(GLenum stage) {
        switch (stage) {
        case GL_VERTEX_SHADER:
            return "VERTEX_SHADER";
        case GL_FRAGMENT_SHADER:
            return "FRAGMENT_SHADER";
        case GL_COMPUTE_SHADER:
            return "COMPUTE_SHADER";
        default:
            throw std::runtime_error("Unsupported shader stage");
        }
    }

    GLuint Compile(const std::string& name, const std::string& source, GLenum stage,
                   std::initializer_list<const char*> defines) {
        // GLSL requires #version first, so the defines go right after it; #line keeps log line numbers.
        auto version = source.find("#version");
        if (version == std::string::npos) {
            throw std::runtime_error(name + ": missing #version directive");
        }
        if (source.find(StageDefine(stage)) == std::string::npos) {
            throw std::runtime_error(name + ": no " + StageDefine(stage) + " section");
        }
        auto body = source.find('\n', version);
        body      = body == std::string::npos ? source.size() : body + 1;
        auto line = std::count(source.begin(), source.begin() + body, '\n') + 1;

        std::string prelude = std::string("#define ") + StageDefine(stage) + "\n";
        for (const auto* define : defines) {
            prelude += std::string("#define ") + define + "\n";
        }
        prelude += "#line " + std::to_string(line) + "\n";

        const GLchar* parts[]{ source.data(), prelude.data(), source.data() + body };
        const GLint lengths[]{ static_cast<GLint>(body), static_cast<GLint>(prelude.size()),
                               static_cast<GLint>(source.size() - body) };

        GLuint shader = glCreateShader(stage);
        glShaderSource(shader, 3, parts, lengths);
        glCompileShader(shader);

        GLint success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLint length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
            std::string message(std::max(length, 1), '\0');
            glGetShaderInfoLog(shader, length, nullptr, message.data());
            glDeleteShader(shader);
            throw std::runtime_error(name + "::" + StageDefine(stage) + ": " + message);
        }

        return shader;
    }

    const char* TypeName(GLenum type) {
        switch (type) {
        case GL_BOOL:
            return "bool";
        case GL_INT:
            return "int";
        case GL_FLOAT:
            return "float";
        case GL_INT_VEC2:
            return "ivec2";
        case GL_FLOAT_VEC2:
            return "vec2";
        case GL_FLOAT_VEC3:
            return "vec3";
        case GL_FLOAT_VEC4:
            return "vec4";
        case GL_FLOAT_MAT2:
            return "mat2";
        case GL_FLOAT_MAT3:
            return "mat3";
        case GL_FLOAT_MAT4:
            return "mat4";
        case GL_SAMPLER_2D:
            return "sampler2D";
        case GL_SAMPLER_CUBE:
            return "samplerCube";
        default:
            return "unsupported uniform type";
        }
    }
} // namespace

Shader::Shader(const std::string& name, std::initializer_list<GLenum> stages, std::initializer_list<const char*> defines) {
    Create(name, stages, defines);
}

Shader::~Shader() {
    glDeleteProgram(id);
}

void Shader::Create(const std::string& name, std::initializer_list<GLenum> stages, std::initializer_list<const char*> defines) {
    if (stages.size() == 0) {
        throw std::runtime_error("At least one shader stage is required");
    }

    const auto path = std::string(DATA_DIRECTORY) + "Shaders/" + name;
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot open shader: " + path);
    }
    std::string source((std::istreambuf_iterator<char>(file)), {});

    GLuint program = glCreateProgram();
    try {
        for (auto stage : stages) {
            GLuint shader = Compile(name, source, stage, defines);
            glAttachShader(program, shader);
            glDeleteShader(shader);
        }

        glLinkProgram(program);
        GLint success = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            GLint length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            std::string message(std::max(length, 1), '\0');
            glGetProgramInfoLog(program, length, nullptr, message.data());
            throw std::runtime_error(name + ": " + message);
        }
    } catch (...) {
        glDeleteProgram(program);
        throw;
    }

    glDeleteProgram(id);
    id = program;
    Reflect();
}

void Shader::Reflect() {
    uniforms.clear();

    GLint count     = 0;
    GLint maxLength = 0;
    glGetProgramiv(id, GL_ACTIVE_UNIFORMS, &count);
    glGetProgramiv(id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);
    std::vector<GLchar> buffer(std::max(maxLength, 1));

    for (GLuint index = 0; index < static_cast<GLuint>(count); ++index) {
        GLsizei length = 0;
        GLint size     = 0;
        GLenum type    = 0;
        glGetActiveUniform(id, index, static_cast<GLsizei>(buffer.size()), &length, &size, &type, buffer.data());
        std::string name(buffer.data(), length);

        // Uniform block members have no location; they are written through buffers.
        GLint location = glGetUniformLocation(id, name.c_str());
        if (location < 0) {
            continue;
        }

        // Arrays are reported once as "name[0]"; register "name" and every "name[i]".
        if (name.ends_with("[0]")) {
            name.resize(name.size() - 3);
            for (GLint element = 0; element < size; ++element) {
                auto elementName      = name + "[" + std::to_string(element) + "]";
                uniforms[elementName] = { type, glGetUniformLocation(id, elementName.c_str()) };
            }
        }
        uniforms[name] = { type, location };
    }
}

void Shader::Bind() const {
    glUseProgram(id);
}

void Shader::Unbind() const {
    glUseProgram(0);
}

void Shader::BindUniformBlock(const std::string& name, unsigned int binding) const {
    GLuint block = glGetUniformBlockIndex(id, name.c_str());
    if (block == GL_INVALID_INDEX) {
        throw std::runtime_error("Uniform block not found: " + name);
    }

    glUniformBlockBinding(id, block, binding);
}

void Shader::SetUniform(const std::string& name, const void* value, GLenum type) const {
    auto found = uniforms.find(name);
    if (found == uniforms.end()) {
        return;
    }

    const auto& uniform    = found->second;
    bool integerCompatible = type == GL_INT &&
                             (uniform.type == GL_BOOL || uniform.type == GL_SAMPLER_2D || uniform.type == GL_SAMPLER_CUBE);
    if (type != uniform.type && !integerCompatible) {
        throw std::runtime_error("Uniform '" + name + "' expects " + TypeName(uniform.type) + ", got " + TypeName(type));
    }

    const auto* ints   = static_cast<const GLint*>(value);
    const auto* floats = static_cast<const GLfloat*>(value);
    switch (type) {
    case GL_INT:
        glProgramUniform1iv(id, uniform.location, 1, ints);
        break;
    case GL_INT_VEC2:
        glProgramUniform2iv(id, uniform.location, 1, ints);
        break;
    case GL_FLOAT:
        glProgramUniform1fv(id, uniform.location, 1, floats);
        break;
    case GL_FLOAT_VEC2:
        glProgramUniform2fv(id, uniform.location, 1, floats);
        break;
    case GL_FLOAT_VEC3:
        glProgramUniform3fv(id, uniform.location, 1, floats);
        break;
    case GL_FLOAT_VEC4:
        glProgramUniform4fv(id, uniform.location, 1, floats);
        break;
    case GL_FLOAT_MAT2:
        glProgramUniformMatrix2fv(id, uniform.location, 1, GL_FALSE, floats);
        break;
    case GL_FLOAT_MAT3:
        glProgramUniformMatrix3fv(id, uniform.location, 1, GL_FALSE, floats);
        break;
    case GL_FLOAT_MAT4:
        glProgramUniformMatrix4fv(id, uniform.location, 1, GL_FALSE, floats);
        break;
    default:
        throw std::runtime_error("Unsupported uniform type: " + name);
    }
}
