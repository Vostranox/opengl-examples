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
#include "imgui.h"
#include <GL/glew.h>

void RunBasicPBR(SceneContext& context) {
    auto& window  = context.window;
    auto& overlay = context.overlay;

    bool rotate = false;

    Renderer renderer;
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glEnable(GL_CULL_FACE));
    GL_CHECK(glFrontFace(GL_CCW));
    GL_CHECK(glCullFace(GL_BACK));

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    LoadModel("cube.obj", vertices, indices);

    VertexArray vao(sizeof(Vertex) * vertices.size(), vertices.data(), sizeof(uint32_t) * indices.size(),
                    indices.data());
    vao.AddAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, position));
    vao.AddAttribute(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, normals));
    vao.AddAttribute(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, texture));

    Shader shader("basic_pbr.glsl");

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

    glm::vec3 albedo   = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 lightPos = glm::vec3(1.0f, 3.0f, 2.0f);
    glm::vec3 lightDir = glm::vec3(0.0f, 0.0f, 1.0f);
    float metallicness = 1.0;
    float roughness    = 0.5;
    float ambient      = 0.5;

    float lightCutoffAngle = 20;
    float innerCone        = 0.9;

    int lightType = 0;

    shader.Set("albedo", albedo);
    shader.Set("metallic", metallicness);
    shader.Set("roughness", roughness);
    shader.Set("ao", ambient);

    shader.Set("roughness", roughness);
    shader.Set("ao", ambient);

    shader.Set("lightCutoffAngle", lightCutoffAngle);
    shader.Set("innerCone", innerCone);

    shader.Set("lightPositions", lightPos);
    shader.Set("lightDir", lightDir);
    shader.Set("camPos", camera.eyePos);

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

        if (rotate) {
            model = glm::rotate(model, glm::radians(1.0f), { 0.0f, 1.0f, 0.0f });
            shader.Set("model", model);
        }

        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        overlay.Begin("Scene controls");

        if (ImGui::DragFloat3("Camera", &camera.eyePos[0], 0.1f)) {
            shader.Set("camPos", camera.eyePos);
            camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);
            shader.Set("lookAt", camera.lookAt);
        }

        if (ImGui::SliderFloat3("lightPos", (float*)&lightPos[0], -10.0f, 10.0f)) {
            shader.Set("lightPositions", lightPos);
        }

        if (ImGui::SliderFloat3("lightDir", (float*)&lightDir[0], -10.0f, 10.0f)) {
            lightDir = glm::normalize(lightDir);
            shader.Set("lightDir", lightDir);
        }

        if (ImGui::SliderFloat3("albedo", (float*)&albedo[0], 0.0f, 1.0f)) {
            shader.Set("albedo", albedo);
        }

        float data[3]{ metallicness, roughness, ambient };
        if (ImGui::SliderFloat3("Stats(metallic, roughness, ambient)", data, 0.0f, 1.0f)) {
            metallicness = data[0];
            roughness    = data[1];
            ambient      = data[2];
            shader.Set("metallic", metallicness);
            shader.Set("roughness", roughness);
            shader.Set("ao", ambient);
        }

        if (ImGui::SliderFloat("LightCutoffAngle", &lightCutoffAngle, 0.0f, 180.0f)) {
            shader.Set("lightCutoffAngle", lightCutoffAngle);
        }

        if (ImGui::SliderFloat("InnerCone", &innerCone, 0.0f, 1.0f)) {
            shader.Set("innerCone", innerCone);
        }

        if (ImGui::SliderInt("Light type", &lightType, 0, 2)) {
            shader.Set("lightType", lightType);
        }

        if (ImGui::Button("Reload shader")) {
            shader.Create("basic_pbr.glsl");
            shader.Bind();
            shader.Set("model", model);
            shader.Set("lookAt", camera.lookAt);
            shader.Set("projection", camera.projec);

            shader.Set("albedo", albedo);
            shader.Set("metallic", metallicness);
            shader.Set("roughness", roughness);
            shader.Set("ao", ambient);

            shader.Set("lightCutoffAngle", lightCutoffAngle);
            shader.Set("innerCone", innerCone);

            shader.Set("lightPositions", lightPos);
            shader.Set("lightDir", lightDir);
            shader.Set("lightType", lightType);

            shader.Set("camPos", camera.eyePos);
        }

        ImGui::Checkbox("Rotate", &rotate);

        overlay.End();

        context.EndFrame();
    }
}
