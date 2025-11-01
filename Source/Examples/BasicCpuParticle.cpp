#include "Examples/Camera.hpp"
#include "Examples/Examples.hpp"
#include "Graphics/SceneResources.hpp"

#include <cmath>
#include <iostream>
#include <numbers>
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
#include "Graphics/Texture.hpp"
#include "Graphics/Utility.hpp"
#include "Graphics/Vertex.hpp"
#include "Graphics/VertexArray.hpp"

namespace {
    const float quadVertices3[] = {
        -1.0f, 1.0f, 0, -1.0f, -1.0f, 0, 1.0f, -1.0f, 0,

        -1.0f, 1.0f, 0, 1.0f, -1.0f, 0, 1.0f, 1.0f, 0
    };

    struct ParticleEmitter {
        void Init(int size) {
            position.resize(size, { 0, 12, 0, 1 });
            position1.resize(size, { 0, 12, 0, 1 });

            velocities.resize(size);
            velocities1.resize(size);
            for (int i = 0; i != position.size(); ++i) {
                velocities[i].x = adh::Random::Real<float>(-8, 6);
                velocities[i].y = adh::Random::Real<float>(1, 6);
                float angle     = adh::Random::Real<float>(0, 2 * std::numbers::pi_v<float>);
                velocities[i] += glm::vec2(cos(angle), sin(angle));

                velocities1[i].x = adh::Random::Real<float>(-5, 5);
                velocities1[i].y = adh::Random::Real<float>(1, 6);
                angle            = adh::Random::Real<float>(0, 2 * std::numbers::pi_v<float>);
                velocities1[i] += glm::vec2(cos(angle), sin(angle));
            }

            quadVAO = resources.VertexArray();
            quadVBO = resources.Buffer();
            glBindVertexArray(quadVAO);
            glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices3), &quadVertices3, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        }

        void Reset() {
            for (int i = 0; i != position.size(); ++i) {
                velocities[i].x = adh::Random::Real<float>(-5, 5);
                velocities[i].y = adh::Random::Real<float>(1, 6);
                float angle     = adh::Random::Real<float>(0, 2 * std::numbers::pi_v<float>);
                velocities[i] += glm::vec2(cos(angle), sin(angle));

                velocities1[i].x = adh::Random::Real<float>(-5, 5);
                velocities1[i].y = adh::Random::Real<float>(1, 6);
                angle            = adh::Random::Real<float>(0, 2 * std::numbers::pi_v<float>);
                velocities1[i] += glm::vec2(cos(angle), sin(angle));

                position[i]  = { adh::Random::Real<float>(-12, 12), adh::Random::Real<float>(6, 16), 0, 1 };
                position1[i] = { adh::Random::Real<float>(-12, 12), adh::Random::Real<float>(6, 16), 0, 1 };
            }
        }

        void Update() {
            for (int i = 0; i != position.size(); ++i) {
                position[i].x += velocities[i].x * 0.08;
                position[i].y += velocities[i].y * 0.08;
                position[i].y -= 0.6;

                position1[i].x += velocities1[i].x * 0.08;
                position1[i].y += velocities1[i].y * 0.08;
                position1[i].y -= 0.6;
            }
        }

        std::vector<glm::vec4> position;
        std::vector<glm::vec4> position1;
        std::vector<glm::vec2> velocities;
        std::vector<glm::vec2> velocities1;
        glm::vec2 gravity{ 0, -0.09 };

        SceneResources resources;
        unsigned int quadVAO = 0;
        unsigned int quadVBO;
    };
} // namespace

void RunBasicCpuParticle(SceneContext& context) {
    auto& window  = context.window;
    auto& overlay = context.overlay;
    SceneResources resources;

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
    model = glm::scale(model, { 0.01, 0.01, 0.01 });

    shader.Bind();
    shader.Set("model", model);
    shader.Set("lookAt", camera.lookAt);
    shader.Set("projection", camera.projec);
    shader.Set("lightZ", lightZ);
    shader.Set("spec", speculat);
    shader.Set("cameraZ", camera.eyePos.z);

    Shader particleShader("particles.glsl");
    particleShader.BindUniformBlock("OffsetBlock", 2);
    particleShader.Bind();
    particleShader.Set("model", model);
    particleShader.Set("lookAt", camera.lookAt);
    particleShader.Set("projection", camera.projec);

    ParticleEmitter particleEmitter;
    particleEmitter.Init(500);

    GLuint particlesUbo = resources.Buffer();
    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, particlesUbo));
    GL_CHECK(glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::vec4) * particleEmitter.position.size(),
                          particleEmitter.position.data(), GL_DYNAMIC_DRAW));
    GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, 2, particlesUbo));

    bool start = false;

    while (context.BeginFrame()) {
        if (window.HasUpdated()) {
            camera.aspect = float(window.GetWidth()) / window.GetHeight();
            camera.projec = glm::perspective(camera.fov, camera.aspect, camera.nearZ, camera.farZ);

            shader.Bind();
            shader.Set("projection", camera.projec);

            particleShader.Bind();
            particleShader.Set("projection", camera.projec);
        }

        renderer.SetClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        renderer.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        particleShader.Bind();
        if (start)
            particleEmitter.Update();

        glBindVertexArray(particleEmitter.quadVAO);

        {
            GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, particlesUbo));
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::vec4) * particleEmitter.position.size(),
                            particleEmitter.position.data());
            GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, 2, particlesUbo));

            glDrawArraysInstanced(GL_TRIANGLES, 0, 6, particleEmitter.position.size());
        }

        {
            GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, particlesUbo));
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::vec4) * particleEmitter.position1.size(),
                            particleEmitter.position1.data());
            GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, 2, particlesUbo));

            glDrawArraysInstanced(GL_TRIANGLES, 0, 6, particleEmitter.position1.size());
        }

        glBindVertexArray(0);

        overlay.Begin("Scene controls");

        ImGui::Checkbox("Run", &start);
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            particleEmitter.Reset();
        }

        if (ImGui::DragFloat("CameraZ", &camera.eyePos.z, 0.1f)) {
            camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);

            shader.Bind();
            shader.Set("cameraZ", camera.eyePos.z);
            shader.Set("lookAt", camera.lookAt);

            particleShader.Bind();
            particleShader.Set("cameraZ", camera.eyePos.z);
            particleShader.Set("lookAt", camera.lookAt);
        }

        overlay.End();

        context.EndFrame();
    }
}
