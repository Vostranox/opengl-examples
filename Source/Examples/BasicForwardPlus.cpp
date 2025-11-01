#include "Examples/Camera.hpp"
#include "Examples/Examples.hpp"
#include "Graphics/SceneResources.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "App/Overlay.hpp"
#include "App/Scene.hpp"
#include "App/Window.hpp"
#include "Examples/Random.hpp"
#include "Graphics/Framebuffer.hpp"
#include "Graphics/Renderbuffer.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/Shader.hpp"
#include "Graphics/Shadowmap.hpp"
#include "Graphics/Texture.hpp"
#include "Graphics/Utility.hpp"
#include "Graphics/Vertex.hpp"
#include "Graphics/VertexArray.hpp"

namespace {
    struct PointLight {
        glm::vec4 color;
        glm::vec4 position;
        glm::vec4 paddingAndRadius;
    };

    struct VisibleIndex {
        int index;
    };
} // namespace

void RunBasicForwardPlus(SceneContext& context) {
    auto& window  = context.window;
    auto& overlay = context.overlay;
    SceneResources resources;

    Renderer renderer;
    glEnable(GL_MULTISAMPLE);
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

    Shader shader("forward_plus.glsl");

    Camera camera;
    camera.nearZ  = 0.1f;
    camera.farZ   = 100.0f;
    camera.aspect = float(window.GetWidth()) / window.GetHeight();
    camera.fov    = glm::radians(90.0f);
    camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);
    camera.projec = glm::perspective(camera.fov, camera.aspect, camera.nearZ, camera.farZ);

    glm::vec3 lightPos{ 0, 1, 7 };

    std::vector<glm::mat4> models;
    float rr = 10.0f;
    for (int i = 0; i != 200; ++i) {
        models.emplace_back(glm::translate(glm::mat4{ 1.0f }, { adh::Random::Real<float>(-rr, rr),
                                                                adh::Random::Real<float>(-rr * 2, rr * 2),
                                                                adh::Random::Real<float>(-rr, rr) }));
    }

    shader.Bind();
    shader.Set("lookAt", camera.lookAt);
    shader.Set("projection", camera.projec);

    shader.Set("viewPos", camera.eyePos);
    shader.Set("lightPos", lightPos);

    Shadowmap shadowMap(window.GetWidth(), window.GetHeight());
    shadowMap.shader->Bind();
    shadowMap.shader->Set("lookAt", camera.lookAt);
    shadowMap.shader->Set("projection", camera.projec);

    const int maxLighNumber = 1024;
    const float lightRadius = 30.0f;

    auto workGroupsX   = (window.GetWidth() + 15) / 16;
    auto workGroupsY   = (window.GetHeight() + 15) / 16;
    auto numberOfTiles = workGroupsX * workGroupsY;

    Shader computeShader("forward_plus.glsl", { GL_COMPUTE_SHADER });
    computeShader.Bind();
    computeShader.Set("depthMap", 4);
    computeShader.Set("lightCount", maxLighNumber);
    glm::ivec2 screenSize(window.GetWidth(), window.GetHeight());
    computeShader.Set("screenSize", screenSize);

    GLuint computeLightBuffer = resources.Buffer();
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeLightBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(PointLight) * maxLighNumber, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, computeLightBuffer);

    PointLight* pointLights = (PointLight*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
    for (int i = 0; i != maxLighNumber; ++i) {
        pointLights[i].position         = glm::vec4(adh::Random::Real<float>(-5, 5), adh::Random::Real<float>(-2000, -5),
                                                    adh::Random::Real<float>(-5, 5), 1);
        pointLights[i].color            = glm::vec4(adh::Random::Real<float>(0, 1), adh::Random::Real<float>(0, 1),
                                                    adh::Random::Real<float>(0, 1), 1);
        pointLights[i].paddingAndRadius = glm::vec4(0, 0, 0, lightRadius);
    }
    lightPos.x = pointLights[0].position.x;
    lightPos.y = pointLights[0].position.y;
    lightPos.z = pointLights[0].position.z;
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    GLuint computeIndexBuffer = resources.Buffer();
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeIndexBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(VisibleIndex) * numberOfTiles * maxLighNumber, nullptr,
                 GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, computeIndexBuffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    while (context.BeginFrame()) {
        if (window.HasUpdated()) {
            camera.aspect = float(window.GetWidth()) / window.GetHeight();
            camera.projec = glm::perspective(camera.fov, camera.aspect, camera.nearZ, camera.farZ);

            shader.Bind();
            shader.Set("projection", camera.projec);

            shadowMap.shader->Bind();
            shadowMap.Recreate(window.GetWidth(), window.GetHeight());
            shadowMap.shader->Set("projection", camera.projec);

            workGroupsX   = (window.GetWidth() + 15) / 16;
            workGroupsY   = (window.GetHeight() + 15) / 16;
            numberOfTiles = workGroupsX * workGroupsY;

            computeShader.Bind();
            computeShader.Set("view", camera.lookAt);
            computeShader.Set("projection", camera.projec);
            glm::ivec2 screenSize(window.GetWidth(), window.GetHeight());
            computeShader.Set("screenSize", screenSize);

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeIndexBuffer);
            glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(VisibleIndex) * numberOfTiles * maxLighNumber, nullptr,
                         GL_STATIC_DRAW);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeLightBuffer);
        PointLight* pointLights = (PointLight*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
        for (int i = 1; i != maxLighNumber; ++i) {
            pointLights[i].position.y += 0.1;
        }
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        renderer.SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        {
            shadowMap.Bind();
            vao.Bind();

            renderer.Clear(GL_DEPTH_BUFFER_BIT);

            for (int i = 0; i != models.size(); ++i) {
                shadowMap.shader->Set("model", models[i]);
                glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            }

            shadowMap.Unbind();
        }

        {
            computeShader.Bind();
            computeShader.Set("view", camera.lookAt);
            computeShader.Set("projection", camera.projec);

            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, shadowMap.texture);

            glDispatchCompute(workGroupsX, workGroupsY, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            renderer.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            vao.Bind();
            shader.Bind();
            shader.Set("numberOfTilesX", workGroupsX);

            for (int i = 0; i != models.size(); ++i) {
                shader.Set("model", models[i]);
                glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            }
        }

        overlay.Begin("Scene controls");

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);

        if (ImGui::DragFloat3("Camera", &camera.eyePos[0], 0.1f)) {
            camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);

            shader.Bind();
            shader.Set("viewPos", camera.eyePos);
            shader.Set("lookAt", camera.lookAt);

            shadowMap.shader->Bind();
            shadowMap.shader->Set("lookAt", camera.lookAt);
        }

        if (ImGui::DragFloat3("lightPos", &lightPos[0], 0.1f)) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, computeLightBuffer);
            PointLight* pointLights   = (PointLight*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
            pointLights[0].position.x = lightPos.x;
            pointLights[0].position.y = lightPos.y;
            pointLights[0].position.z = lightPos.z;
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }

        ImGui::Image(static_cast<ImTextureID>(shadowMap.texture), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));

        overlay.End();

        context.EndFrame();
    }
}
