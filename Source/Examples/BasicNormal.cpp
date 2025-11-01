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

void RunBasicNormal(SceneContext& context) {
    auto& window  = context.window;
    auto& overlay = context.overlay;

    float lightZ       = 7.0f;
    float speculat     = 0.5f;
    glm::vec3 lightPos = glm::vec3(0, 0, 7);

    Renderer renderer;
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glEnable(GL_CULL_FACE));
    GL_CHECK(glFrontFace(GL_CCW));
    GL_CHECK(glCullFace(GL_BACK));

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    LoadModel("plane.obj", vertices, indices);

    VertexArray vao(sizeof(Vertex) * vertices.size(), vertices.data(), sizeof(uint32_t) * indices.size(),
                    indices.data());
    vao.AddAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, position));
    vao.AddAttribute(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, normals));
    vao.AddAttribute(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, texture));
    vao.AddAttribute(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, tangent));
    vao.AddAttribute(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, bitangent));

    Shader shader("basic_normal_mapping.glsl");

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

    shader.Set("lightPos", lightPos);
    shader.Set("viewPos", camera.eyePos);

    Texture texture01(0, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR, "test/diffuse.png", 0);
    Texture texture02(1, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR, "test/normal.png", 0);
    Texture texture03(2, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR, "test/displacement.png", 0);
    shader.Set("texture01", 0);
    shader.Set("texture02", 1);
    shader.Set("texture03", 2);

    float heightScale = 0;
    shader.Set("heightScale", heightScale);

    bool useNormalMap = true;
    shader.Set("useNormalMap", useNormalMap);

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
        texture01.Bind();
        texture02.Bind();
        texture03.Bind();
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        overlay.Begin("Scene controls");

        if (ImGui::DragFloat3("Camera", &camera.eyePos[0], 0.01f, -10.0f, 10.0f)) {
            shader.Set("viewPos", camera.eyePos);
            camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);
            shader.Set("lookAt", camera.lookAt);
        }

        if (ImGui::Checkbox("Use normal map", &useNormalMap)) {
            shader.Set("useNormalMap", useNormalMap);
        }

        if (ImGui::DragFloat3("Light pos", &lightPos[0], 0.1f, -10.0f, 10.0f)) {
            shader.Set("lightPos", lightPos);
        }

        if (ImGui::DragFloat("specularStrenght", &speculat, 0.1f, 0.0f, 10.0f)) {
            shader.Set("spec", speculat);
        }

        if (ImGui::DragFloat("heightScale", &heightScale, 0.01f, -2.0f, 2.0f)) {
            shader.Set("heightScale", heightScale);
        }

        ImGui::Image(static_cast<ImTextureID>(texture01.GetID()), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::SameLine();
        ImGui::Image(static_cast<ImTextureID>(texture02.GetID()), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::SameLine();
        ImGui::Image(static_cast<ImTextureID>(texture03.GetID()), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::SameLine();

        overlay.End();

        context.EndFrame();
    }
}
