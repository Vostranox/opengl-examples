#include "Examples/Camera.hpp"
#include "Examples/Examples.hpp"

#include <iostream>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "App/Overlay.hpp"
#include "App/Scene.hpp"
#include "App/Window.hpp"
#include "Graphics/Framebuffer.hpp"
#include "Graphics/Renderbuffer.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/Shader.hpp"
#include "Graphics/Texture.hpp"
#include "Graphics/Utility.hpp"
#include "Graphics/Vertex.hpp"
#include "Graphics/VertexArray.hpp"

void RunBasicLight(SceneContext& context) {
    auto& window  = context.window;
    auto& overlay = context.overlay;

    float lightZ   = 7.0f;
    float speculat = 0.5f;

    Renderer renderer;
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glEnable(GL_CULL_FACE));
    GL_CHECK(glFrontFace(GL_CCW));
    GL_CHECK(glCullFace(GL_BACK));

    GL_CHECK(glEnable(GL_BLEND));
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    LoadModel("cube.obj", vertices, indices);

    VertexArray vao(sizeof(Vertex) * vertices.size(), vertices.data(), sizeof(uint32_t) * indices.size(),
                    indices.data());
    vao.AddAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, position));
    vao.AddAttribute(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, normals));
    vao.AddAttribute(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, texture));

    Shader shader("light.glsl");

    Camera camera;
    camera.nearZ  = 0.1f;
    camera.farZ   = 100.0f;
    camera.aspect = float(window.GetWidth()) / window.GetHeight();
    camera.fov    = glm::radians(90.0f);
    camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);
    camera.projec = glm::perspective(camera.fov, camera.aspect, camera.nearZ, camera.farZ);

    glm::mat4 model{ 1.0f };

    shader.Bind();
    shader.Set("model", model);
    shader.Set("lookAt", camera.lookAt);
    shader.Set("projection", camera.projec);
    shader.Set("lightZ", lightZ);
    shader.Set("spec", speculat);
    shader.Set("cameraZ", camera.eyePos.z);

    while (context.BeginFrame()) {
        if (window.HasUpdated()) {
            camera.aspect = float(window.GetWidth()) / window.GetHeight();
            camera.projec = glm::perspective(camera.fov, camera.aspect, camera.nearZ, camera.farZ);

            shader.Bind();
            shader.Set("projection", camera.projec);
        }

        renderer.SetClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        renderer.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        vao.Bind();
        shader.Bind();

        model = glm::rotate(model, glm::radians(1.0f), { 0.0f, 1.0f, 0.0f });
        model = glm::translate(model, { 0.1f, 0.0f, 0.0f });
        shader.Set("model", model);

        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        overlay.Begin("Scene controls");

        if (ImGui::DragFloat("CameraZ", &camera.eyePos.z, 0.1f)) {
            shader.Set("cameraZ", camera.eyePos.z);
            camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);
            shader.Set("lookAt", camera.lookAt);
        }

        if (ImGui::DragFloat("LightZ", &lightZ, 0.1f)) {
            shader.Set("lightZ", lightZ);
        }

        if (ImGui::DragFloat("specularStrenght", &speculat, 0.1f, 0.0f, 10.0f)) {
            shader.Set("spec", speculat);
        }

        overlay.End();

        context.EndFrame();
    }
}
