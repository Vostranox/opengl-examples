#include <GL/glew.h>

#define STB_IMAGE_IMPLEMENTATION
#include "Graphics/Utility.hpp"

#include <sstream>

void CheckOpenGLErrors(const char* expression, bool pending, std::source_location location) {
    GLenum error = glGetError();
    if (error == GL_NO_ERROR) {
        return;
    }

    std::ostringstream message;
    message << (pending ? "Pending OpenGL error before checked call " : "OpenGL error from checked call ") << expression
            << " at " << location.file_name() << ':' << location.line() << " (" << location.function_name() << "):";
    do {
        message << " 0x" << std::hex << error;
        error = glGetError();
    } while (error != GL_NO_ERROR);

    throw std::runtime_error(message.str());
}
