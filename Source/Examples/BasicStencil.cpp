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

void RunBasicStencil(SceneContext& context) {
    auto& window  = context.window;
    auto& overlay = context.overlay;

    float lightZ   = 7.0f;
    float speculat = 0.5f;

    Renderer renderer;
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glEnable(GL_CULL_FACE));
    GL_CHECK(glFrontFace(GL_CCW));
    GL_CHECK(glCullFace(GL_BACK));

    GL_CHECK(glEnable(GL_STENCIL_TEST));
    GL_CHECK(glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE));

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    LoadModel("cube.obj", vertices, indices);

    VertexArray vao(sizeof(Vertex) * vertices.size(), vertices.data(), sizeof(uint32_t) * indices.size(),
                    indices.data());
    vao.AddAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, position));
    vao.AddAttribute(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, normals));
    vao.AddAttribute(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, texture));

    Shader shader("basic_stencil.glsl");

    Camera camera;
    camera.nearZ  = 0.1f;
    camera.farZ   = 100.0f;
    camera.aspect = float(window.GetWidth()) / window.GetHeight();
    camera.fov    = glm::radians(90.0f);
    camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);
    camera.projec = glm::perspective(camera.fov, camera.aspect, camera.nearZ, camera.farZ);

    glm::vec3 pos{};
    glm::mat4 model{ 1.0f };
    model = glm::scale(model, { 0.4, 0.4, 0.4 });

    glm::mat4 model2{ 1.0f };
    model2 = glm::scale(model2, { 0.42, 0.42, 0.42 });

    shader.Bind();
    shader.Set("model", model);
    shader.Set("lookAt", camera.lookAt);
    shader.Set("projection", camera.projec);
    shader.Set("lightZ", lightZ);
    shader.Set("spec", speculat);
    shader.Set("cameraZ", camera.eyePos);

    while (context.BeginFrame()) {
        if (window.HasUpdated()) {
            camera.aspect = float(window.GetWidth()) / window.GetHeight();
            camera.projec = glm::perspective(camera.fov, camera.aspect, camera.nearZ, camera.farZ);

            shader.Bind();
            shader.Set("projection", camera.projec);
        }

        renderer.SetClearColor(0.2f, 0.2f, 0.2f, 1.0f);

        glStencilMask(0xFF);
        renderer.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        vao.Bind();
        shader.Bind();
        {
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glEnable(GL_DEPTH_TEST);

            shader.Set("model", model);
            shader.Set("isStencil", 0);

            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        }

        {
            glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
            glStencilMask(0x00);
            glDisable(GL_DEPTH_TEST);

            shader.Set("model", model2);
            shader.Set("isStencil", 1);

            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        }

        overlay.Begin("Scene controls");

        if (ImGui::DragFloat3("CameraZ", &camera.eyePos[0], 0.1f, -20.f, 20.f)) {
            shader.Set("cameraZ", camera.eyePos);
            camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);
            shader.Set("lookAt", camera.lookAt);
        }

        if (ImGui::DragFloat3("pos", &pos[0], 0.1f, -2.0f, 2.0f)) {
            model2 = glm::translate(glm::mat4{ 1.0f }, pos);
            model2 = glm::scale(model2, { 0.42, 0.42, 0.42 });
            shader.Set("model", model2);
            shader.Set("spec", speculat);
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
