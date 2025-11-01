#include "Examples/Camera.hpp"
#include "Examples/Examples.hpp"
#include "Graphics/SceneResources.hpp"

#include <cstring>
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

void RunBasicCompute(SceneContext& context) {
    auto& window  = context.window;
    auto& overlay = context.overlay;
    SceneResources resources;

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

    Shader shader("compute.glsl");

    Camera camera;
    camera.nearZ  = 0.1f;
    camera.farZ   = 100.0f;
    camera.aspect = float(window.GetWidth()) / window.GetHeight();
    camera.fov    = glm::radians(90.0f);
    camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);
    camera.projec = glm::perspective(camera.fov, camera.aspect, camera.nearZ, camera.farZ);

    glm::vec3 lightPos{ 0, 1, 7 };

    glm::mat4 model{ 1.0f };

    shader.Bind();
    shader.Set("model", model);
    shader.Set("lookAt", camera.lookAt);
    shader.Set("projection", camera.projec);

    shader.Set("viewPos", camera.eyePos);
    shader.Set("lightPos", lightPos);
    shader.Set("shadowMap", 0);

    Shader computeShader("compute.glsl", { GL_COMPUTE_SHADER });

    const int sumSize = 16;
    int values[sumSize]{};
    GLuint computeBuffer = resources.Buffer();
    GL_CHECK(glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffer));
    GL_CHECK(glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(values), values, GL_DYNAMIC_READ));
    GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, computeBuffer));

    Shadowmap shadowMap(window.GetWidth(), window.GetHeight());
    shadowMap.shader->Bind();
    shadowMap.shader->Set("model", model);
    shadowMap.shader->Set("lookAt", camera.lookAt);
    shadowMap.shader->Set("projection", camera.projec);

    while (context.BeginFrame()) {
        if (window.HasUpdated()) {
            camera.aspect = float(window.GetWidth()) / window.GetHeight();
            camera.projec = glm::perspective(camera.fov, camera.aspect, camera.nearZ, camera.farZ);

            shader.Bind();
            shader.Set("projection", camera.projec);

            shadowMap.shader->Bind();
            shadowMap.Recreate(window.GetWidth(), window.GetHeight());
            shadowMap.shader->Set("projection", camera.projec);
        }

        renderer.SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        {
            shadowMap.Bind();
            vao.Bind();
            renderer.Clear(GL_DEPTH_BUFFER_BIT);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            shadowMap.Unbind();
        }

        computeShader.Bind();
        glDispatchCompute(2, 2, 1);
        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeBuffer);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(values), values);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            renderer.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            vao.Bind();
            shader.Bind();
            shader.Set("model", model);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, shadowMap.texture);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        }

        overlay.Begin("Scene controls");

        ImGui::TextUnformatted("Compute results (4 x 4 invocations)");
        for (int row = 0; row < 4; ++row) {
            ImGui::Text("%2d  %2d  %2d  %2d", values[row * 4], values[row * 4 + 1], values[row * 4 + 2],
                        values[row * 4 + 3]);
        }

        if (ImGui::DragFloat3("Camera", &camera.eyePos[0], 0.1f)) {
            camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);

            shader.Bind();
            shader.Set("viewPos", camera.eyePos);
            shader.Set("lookAt", camera.lookAt);

            shadowMap.shader->Bind();
            shadowMap.shader->Set("lookAt", camera.lookAt);
        }

        if (ImGui::DragFloat3("lightPos", &lightPos[0], 0.1f)) {
            shader.Bind();
            shader.Set("lightPos", lightPos);
        }

        ImGui::Image(static_cast<ImTextureID>(shadowMap.texture), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));

        overlay.End();

        context.EndFrame();
    }
}
