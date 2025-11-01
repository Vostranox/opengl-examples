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
#include "Graphics/Shadowmap.hpp"
#include "Graphics/Texture.hpp"
#include "Graphics/Utility.hpp"
#include "Graphics/Vertex.hpp"
#include "Graphics/VertexArray.hpp"

void RunBasicShadow(SceneContext& context) {
    auto& window  = context.window;
    auto& overlay = context.overlay;

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

    Shader shader("shadow.glsl");

    Camera camera;
    camera.nearZ  = 0.1f;
    camera.farZ   = 100.0f;
    camera.aspect = float(window.GetWidth()) / window.GetHeight();
    camera.fov    = glm::radians(90.0f);
    camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);
    camera.projec = glm::perspective(camera.fov, camera.aspect, camera.nearZ, camera.farZ);

    glm::vec3 lightPos{ 0, 1, 7 };
    glm::mat4 lightProj = glm::perspective(glm::radians(140.0f), camera.aspect, camera.nearZ, camera.farZ);
    glm::mat4 lightView = glm::lookAt(lightPos, camera.focus, camera.upVector);

    glm::mat4 model{ 1.0f };
    glm::mat4 model2{ 1.0f };

    model2 = glm::translate(model2, { 0, -4, 0 });
    model2 = glm::scale(model2, { 10, 1, 10 });

    shader.Bind();
    shader.Set("model", model);
    shader.Set("lookAt", camera.lookAt);
    shader.Set("projection", camera.projec);
    shader.Set("lightProjView", lightProj * lightView);

    shader.Set("viewPos", camera.eyePos);
    shader.Set("lightPos", lightPos);
    shader.Set("shadowMap", 0);

    Shadowmap shadowMap(window.GetWidth(), window.GetHeight());
    shadowMap.shader->Bind();
    shadowMap.shader->Set("model", model);
    shadowMap.shader->Set("lookAt", lightView);
    shadowMap.shader->Set("projection", lightProj);

    while (context.BeginFrame()) {
        if (window.HasUpdated()) {
            camera.aspect = float(window.GetWidth()) / window.GetHeight();
            camera.projec = glm::perspective(camera.fov, camera.aspect, camera.nearZ, camera.farZ);

            shader.Bind();
            shader.Set("projection", camera.projec);

            lightProj = glm::perspective(glm::radians(140.0f), camera.aspect, camera.nearZ, camera.farZ);
            shader.Set("lightProjView", lightProj * lightView);
            shadowMap.Recreate(window.GetWidth(), window.GetHeight());
            shadowMap.shader->Bind();
            shadowMap.shader->Set("projection", lightProj);
        }

        renderer.SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        {
            shadowMap.Bind();
            vao.Bind();
            renderer.Clear(GL_DEPTH_BUFFER_BIT);
            shadowMap.shader->Set("model", model);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            shadowMap.shader->Set("model", model2);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        }

        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            renderer.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            vao.Bind();
            shader.Bind();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, shadowMap.texture);

            shader.Set("model", model);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            shader.Set("model", model2);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        }

        overlay.Begin("Scene controls");

        if (ImGui::DragFloat3("Camera", &camera.eyePos[0], 0.1f)) {
            camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);
            shader.Set("viewPos", camera.eyePos);
            shader.Set("lookAt", camera.lookAt);
        }

        if (ImGui::DragFloat3("lightPos", &lightPos[0], 0.1f)) {
            lightView = glm::lookAt(lightPos, camera.focus, camera.upVector);

            shader.Bind();
            shader.Set("lightPos", lightPos);
            shader.Set("lightProjView", lightProj * lightView);

            shadowMap.shader->Bind();
            shadowMap.shader->Set("lookAt", lightView);
            shadowMap.shader->Unbind();
        }

        ImGui::Image(static_cast<ImTextureID>(shadowMap.texture), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));

        overlay.End();

        context.EndFrame();
    }
}
