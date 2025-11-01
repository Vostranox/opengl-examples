#include "App/Scene.hpp"

SceneContext::SceneContext(Window& window, Overlay& overlay, const char* name)
    : window(window), overlay(overlay), name(name) {}

bool SceneContext::BeginFrame() {
    if (back || !window.IsOpen()) {
        return false;
    }

    window.PollEvents();
    while (window.IsOpen() && (window.GetWidth() == 0 || window.GetHeight() == 0)) {
        glfwWaitEventsTimeout(0.05);
        window.PollEvents();
    }

    if (!window.IsOpen()) {
        return false;
    }

    overlay.BeginFrame();
    if (name && ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
        overlay.CancelFrame();
        return false;
    }

    return true;
}

void SceneContext::EndFrame() {
    if (ImGui::BeginMainMenuBar()) {
        if (name) {
            if (ImGui::Button("Back to scenes")) {
                back = true;
            }
            ImGui::SameLine();
            ImGui::Text("%s | Esc to return", name);
        } else {
            ImGui::TextUnformatted("OpenGL examples");
        }
        ImGui::EndMainMenuBar();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, window.GetWidth(), window.GetHeight());

    overlay.EndFrame();
    window.SwapBuffers();
}

void ResetGraphicsState(Window& window) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    for (int binding = 0; binding < 3; ++binding) {
        glBindBufferBase(GL_UNIFORM_BUFFER, binding, 0);
    }

    if (window.Supports(4, 3)) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
    }

    glUseProgram(0);

    for (int unit = 0; unit < 5; ++unit) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }
    glActiveTexture(GL_TEXTURE0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glViewport(0, 0, window.GetWidth(), window.GetHeight());
}
