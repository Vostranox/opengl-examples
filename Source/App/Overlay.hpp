#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

class Overlay {
  public:
    Overlay(GLFWwindow* window) {
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui::GetIO().IniFilename = nullptr;

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 410");

        ImGui::GetStyle().FontScaleMain = 1.5f;
    }

    void BeginFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        frameActive = true;
    }

    void EndFrame() {
        ImGui::Render();
        frameActive = false;
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void CancelFrame() {
        if (frameActive) {
            while (windowDepth > 0) {
                End();
            }

            ImGui::EndFrame();
            frameActive = false;
        }
    }

    bool Begin(const char* label, ImVec2 size = ImVec2(520, 400)) {
        ++windowDepth;
        ImGui::SetNextWindowPos(ImVec2(16, 64), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
        return ImGui::Begin(label);
    }

    void End() {
        ImGui::End();
        --windowDepth;
    }

    bool Checkbox(const char* name, bool& value) { return ImGui::Checkbox(name, &value); }

    void DrawFPS() {
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);
    }

    ~Overlay() {
        CancelFrame();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

  private:
    bool frameActive = false;
    int windowDepth  = 0;
};
