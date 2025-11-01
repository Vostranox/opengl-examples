#include "App/Scene.hpp"
#include "Examples/Examples.hpp"

#include <array>
#include <functional>
#include <iostream>
#include <optional>
#include <string>

namespace {
    struct Scene {
        std::string name;
        std::function<void(SceneContext&)> run;
    };

    using SceneSelection = std::optional<std::reference_wrapper<const Scene>>;

    const auto scenes = std::to_array<Scene>({
        { "Lighting", RunBasicLight },
        { "Stencil outlines", RunBasicStencil },
        { "Instancing", RunBasicInstance },
        { "Post-processing", RunBasicPostprocess },
        { "Physically based rendering", RunBasicPBR },
        { "Shadows", RunBasicShadow },
        { "Cubemap", RunBasicCubemap },
        { "Deferred rendering", RunBasicDeferred },
        { "Normal mapping", RunBasicNormal },
        { "Bloom", RunBasicBloom },
        { "CPU particles", RunBasicCpuParticle },
        { "Compute", RunBasicCompute },
        { "Forward plus", RunBasicForwardPlus },
    });

    SceneSelection DrawSceneSelector(Overlay& overlay, const std::string& error) {
        SceneSelection selected;
        overlay.Begin("Scenes", ImVec2(520, 0));

        ImGui::Text("OpenGL %s", glGetString(GL_VERSION));
        ImGui::TextUnformatted("Choose an example. Press Esc to return here.");
        ImGui::Separator();

        for (const auto& scene : scenes) {
            if (ImGui::Selectable(scene.name.c_str(), false, 0, ImVec2(0, 30))) {
                selected = std::cref(scene);
            }
        }

        if (!error.empty()) {
            ImGui::Separator();
            ImGui::TextWrapped("%s", error.c_str());
        }

        overlay.End();
        return selected;
    }
} // namespace

int main() {
    try {
        Window window;
        window.CreateWindow(1100, 850, "OpenGL examples");

        Overlay overlay(window.GetHandle());
        SceneContext menu(window, overlay);

        std::string error;

        while (menu.BeginFrame()) {
            ResetGraphicsState(window);
            glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            const auto selected = DrawSceneSelector(overlay, error);
            menu.EndFrame();

            if (selected) {
                const auto& scene = selected->get();
                error.clear();
                ResetGraphicsState(window);
                SceneContext context(window, overlay, scene.name.c_str());
                try {
                    scene.run(context);
                } catch (const std::exception& exception) {
                    overlay.CancelFrame();
                    error = scene.name + ": " + exception.what();
                    std::cerr << error << std::endl;
                }
                ResetGraphicsState(window);
            }
        }
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << std::endl;
        return 1;
    }
    return 0;
}
