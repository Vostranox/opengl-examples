#pragma once

#include <iostream>
#include <stdexcept>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

class Window {
  public:
    Window() {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }
    }

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    ~Window() {
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }

    void CreateWindow(int width, int height, const char* name) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
        glfwWindowHint(GLFW_SAMPLES, 8);
        glfwWindowHint(GLFW_STENCIL_BITS, 8);

        const int versions[] = { 6, 3, 1 };
        for (int minor : versions) {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
            window = glfwCreateWindow(width, height, name, nullptr, nullptr);
            if (window) {
                break;
            }
        }

        if (!window) {
            throw std::runtime_error("OpenGL 4.1 or newer is required");
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        glewExperimental    = GL_TRUE;
        const GLenum status = glewInit();
        if (status != GLEW_OK && !(status == GLEW_ERROR_NO_GLX_DISPLAY && glfwGetPlatform() == GLFW_PLATFORM_WAYLAND)) {
            throw std::runtime_error(std::string("Failed to initialize GLEW: ") +
                                     reinterpret_cast<const char*>(glewGetErrorString(status)));
        }

        while (glGetError() != GL_NO_ERROR) {
        }

        glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
        glGetIntegerv(GL_MINOR_VERSION, &minorVersion);

        glfwGetFramebufferSize(window, &this->width, &this->height);
        glViewport(0, 0, this->width, this->height);

        std::cout << glGetString(GL_VENDOR) << '\n'
                  << glGetString(GL_RENDERER) << '\n'
                  << glGetString(GL_VERSION) << std::endl;
    }

    bool IsOpen() const { return !glfwWindowShouldClose(window); }

    void SwapBuffers() { glfwSwapBuffers(window); }

    void PollEvents() {
        glfwPollEvents();

        int currentWidth, currentHeight;
        glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
        resized = currentWidth != width || currentHeight != height;
        width   = currentWidth;
        height  = currentHeight;

        if (resized) {
            glViewport(0, 0, width, height);
        }
    }

    GLFWwindow* GetHandle() const { return window; }

    int GetWidth() const { return width; }

    int GetHeight() const { return height; }

    bool HasUpdated() const { return resized; }

    bool Supports(int major, int minor) const {
        return majorVersion > major || (majorVersion == major && minorVersion >= minor);
    }

  private:
    GLFWwindow* window = nullptr;
    int width          = 0;
    int height         = 0;
    int majorVersion   = 0;
    int minorVersion   = 0;
    bool resized       = false;
};
