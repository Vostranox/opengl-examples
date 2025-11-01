#include "Examples/Camera.hpp"
#include "Examples/Examples.hpp"
#include "Examples/Geometry.hpp"
#include "Graphics/SceneResources.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "App/Overlay.hpp"
#include "App/Window.hpp"
#include "Graphics/Framebuffer.hpp"
#include "Graphics/Renderbuffer.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/Shader.hpp"
#include "Graphics/Texture.hpp"
#include "Graphics/Utility.hpp"
#include "Graphics/Vertex.hpp"
#include "Graphics/VertexArray.hpp"

#include "App/Scene.hpp"
#include "Examples/Random.hpp"

void RunBasicDeferred(SceneContext& context) {
    auto& window  = context.window;
    auto& overlay = context.overlay;
    SceneResources resources;

    float metallic     = 1.0;
    float roughness    = 0.5;
    bool lightVolumes  = true;
    glm::vec3 lightDir = glm::vec3(0.0f, 0.0f, 1.0f);

    std::vector<glm::mat4> models;
    std::vector<glm::vec3> albedo;
    std::vector<glm::vec4> lightPos;
    std::vector<glm::vec4> lightColor;

    Renderer renderer;
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glEnable(GL_CULL_FACE));
    GL_CHECK(glFrontFace(GL_CCW));
    GL_CHECK(glCullFace(GL_BACK));

    glDisable(GL_BLEND);

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    LoadModel("sphere.obj", vertices, indices);

    float meshInnerRadius = std::numeric_limits<float>::max();
    for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
        glm::vec3 a = vertices[indices[i]].position;
        glm::vec3 n = glm::cross(vertices[indices[i + 1]].position - a, vertices[indices[i + 2]].position - a);
        if (glm::length(n) > 0.0f) {
            meshInnerRadius = std::min(meshInnerRadius, std::abs(glm::dot(glm::normalize(n), a)));
        }
    }

    VertexArray vao(sizeof(Vertex) * vertices.size(), vertices.data(), sizeof(uint32_t) * indices.size(),
                    indices.data());
    vao.AddAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, position));
    vao.AddAttribute(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, normals));
    vao.AddAttribute(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, texture));

    Shader shader("light_deferred.glsl");
    Shader volumeShader("light_vol_deferred.glsl");
    Shader resolveShader("light_vol_resolve.glsl");
    VertexArray vao2(sizeof(quadVertices2), quadVertices2, 0, nullptr);
    vao2.AddAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0);
    vao2.AddAttribute(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, sizeof(float) * 3);

    Camera camera;
    camera.eyePos = glm::vec3(0.0f, 0.0f, 12.0f);
    camera.nearZ  = 0.1f;
    camera.farZ   = 1000.0f;
    camera.aspect = float(window.GetWidth()) / window.GetHeight();
    camera.fov    = glm::radians(90.0f);
    camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);
    camera.projec = glm::perspective(camera.fov, camera.aspect, camera.nearZ, camera.farZ);

    shader.Bind();
    {
        float rr = 5.0f;

        for (int i = 0; i != 100; ++i) {
            models.emplace_back(
                glm::translate(glm::mat4{ 1.0f }, { adh::Random::Real<float>(-rr, rr), adh::Random::Real<float>(-rr, rr),
                                                    adh::Random::Real<float>(-rr, rr) }));

            albedo.emplace_back(glm::vec3{ 1, 1, 1 });
        }

        rr *= 2;
        for (int i = 0; i != 500; ++i) {
            lightPos.emplace_back(glm::vec4(adh::Random::Real<float>(-rr, rr), adh::Random::Real<float>(-rr, rr),
                                            adh::Random::Real<float>(-rr, rr), 1.0f));
            lightColor.emplace_back(glm::vec4(adh::Random::Real<float>(0, 1), adh::Random::Real<float>(0, 1),
                                              adh::Random::Real<float>(0, 1), 1.0f));
        }
    }

    const auto lightBytes = static_cast<GLsizeiptr>(sizeof(glm::vec4) * lightPos.size());
    GLuint lightUbo       = resources.Buffer();
    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, lightUbo));
    GL_CHECK(glBufferData(GL_UNIFORM_BUFFER, 2 * lightBytes, nullptr, GL_DYNAMIC_DRAW));
    GL_CHECK(glBufferSubData(GL_UNIFORM_BUFFER, 0, lightBytes, lightPos.data()));
    GL_CHECK(glBufferSubData(GL_UNIFORM_BUFFER, lightBytes, lightBytes, lightColor.data()));
    GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, 0, lightUbo));
    shader.BindUniformBlock("LightBlock", 0);
    volumeShader.BindUniformBlock("LightBlock", 0);

    shader.Set("gPosition", 0);
    shader.Set("gNormal", 1);
    shader.Set("gAlbedoSpec", 2);
    shader.Set("lightDir", lightDir);
    shader.Set("camPos", camera.eyePos);

    volumeShader.Set("gPosition", 0);
    volumeShader.Set("gNormal", 1);
    volumeShader.Set("gAlbedoSpec", 2);
    volumeShader.Set("camPos", camera.eyePos);
    volumeShader.Set("view", camera.lookAt);
    volumeShader.Set("projection", camera.projec);

    resolveShader.Set("gAlbedoSpec", 2);
    resolveShader.Set("lightAccum", 3);

    GLuint gBuffer = resources.Framebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    unsigned int gPosition, gNormal, gColorSpec;

    gPosition = resources.Texture();
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window.GetWidth(), window.GetHeight(), 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    gNormal = resources.Texture();
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window.GetWidth(), window.GetHeight(), 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    gColorSpec = resources.Texture();
    glBindTexture(GL_TEXTURE_2D, gColorSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window.GetWidth(), window.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gColorSpec, 0);

    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    GLuint rbo = resources.Renderbuffer();
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window.GetWidth(), window.GetHeight());
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Framebuffer not complete!");

    GLuint lightAccum = resources.Texture();
    glBindTexture(GL_TEXTURE_2D, lightAccum);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window.GetWidth(), window.GetHeight(), 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLuint accumBuffer = resources.Framebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, accumBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightAccum, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Framebuffer not complete!");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    Shader deferredShader("basic_deferred.glsl");
    deferredShader.Bind();
    deferredShader.Set("view", camera.lookAt);
    deferredShader.Set("projection", camera.projec);
    deferredShader.Set("metallic", metallic);
    deferredShader.Set("roughness", roughness);

    Shader shader2("post.glsl");
    shader2.Bind();
    shader2.Set("texture01", 0);
    shader2.Unbind();

    Texture fbo_texture(0, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR, GL_RGBA, window.GetWidth(), window.GetHeight());
    Framebuffer fbo;
    fbo.Bind();
    fbo.BindTexture(fbo_texture, GL_COLOR_ATTACHMENT0);
    fbo.BindRenderbuffer(rbo, GL_DEPTH_ATTACHMENT);
    fbo.Unbind();

    float constant  = 1.0;
    float linear    = 0.7;
    float quadratic = 1.8;
    float lightMax  = std::fmaxf(std::fmaxf(1, 1), 1);
    float radius =
        (-linear + sqrtf(linear * linear - 4 * quadratic * (constant - (256.0 / 5.0) * lightMax))) / (2 * quadratic);
    glm::vec3 attenuation(constant, linear, quadratic);
    shader.Bind();
    shader.Set("radius", radius);
    shader.Set("attenuation", attenuation);
    volumeShader.Set("radius", radius);
    volumeShader.Set("attenuation", attenuation);
    volumeShader.Set("volumeScale", radius / meshInnerRadius);

    while (context.BeginFrame()) {
        if (window.HasUpdated()) {
            camera.aspect = float(window.GetWidth()) / window.GetHeight();
            camera.projec = glm::perspective(camera.fov, camera.aspect, camera.nearZ, camera.farZ);

            deferredShader.Bind();
            deferredShader.Set("projection", camera.projec);
            volumeShader.Set("projection", camera.projec);

            glBindTexture(GL_TEXTURE_2D, gPosition);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window.GetWidth(), window.GetHeight(), 0, GL_RGBA, GL_FLOAT,
                         NULL);

            glBindTexture(GL_TEXTURE_2D, gNormal);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window.GetWidth(), window.GetHeight(), 0, GL_RGBA, GL_FLOAT,
                         NULL);

            glBindTexture(GL_TEXTURE_2D, gColorSpec);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window.GetWidth(), window.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_INT,
                         NULL);

            glBindTexture(GL_TEXTURE_2D, lightAccum);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window.GetWidth(), window.GetHeight(), 0, GL_RGBA, GL_FLOAT,
                         NULL);
            glBindTexture(GL_TEXTURE_2D, 0);

            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window.GetWidth(), window.GetHeight());
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            glBindTexture(GL_TEXTURE_2D, fbo_texture);
            GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window.GetWidth(), window.GetHeight(), 0, GL_RGBA,
                                  GL_UNSIGNED_BYTE, nullptr));
            GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
        }

        renderer.SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        {
            glDepthMask(GL_TRUE);
            renderer.Enable(GL_DEPTH_TEST);

            glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
            renderer.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gPosition);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gNormal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, gColorSpec);

            vao.Bind();
            deferredShader.Bind();

            for (int i = 0; i < models.size(); ++i) {
                deferredShader.Set("model", models[i]);
                deferredShader.Set("albedo", albedo[i]);
                deferredShader.Set("metallic", metallic);
                deferredShader.Set("roughness", roughness);
                glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            }
        }

        {
            glDepthMask(GL_FALSE);

            if (lightVolumes) {
                glBindFramebuffer(GL_FRAMEBUFFER, accumBuffer);
                renderer.Clear(GL_COLOR_BUFFER_BIT);
                glDepthFunc(GL_GEQUAL);
                glCullFace(GL_FRONT);
                renderer.Enable(GL_BLEND);
                glBlendFunc(GL_ONE, GL_ONE);

                vao.Bind();
                volumeShader.Bind();
                glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, lightPos.size());

                renderer.Disable(GL_BLEND);
                glCullFace(GL_BACK);
                glDepthFunc(GL_LESS);
            }

            renderer.Disable(GL_DEPTH_TEST);
            fbo.Bind();
            renderer.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            vao2.Bind();
            if (lightVolumes) {
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, lightAccum);
                resolveShader.Bind();
            } else {
                shader.Bind();
            }
            glDrawArrays(GL_TRIANGLES, 0, 6);
            fbo.Unbind();
        }

        {
            renderer.Disable(GL_DEPTH_TEST);
            renderer.Clear(GL_COLOR_BUFFER_BIT);

            shader2.Bind();
            fbo_texture.Bind();
            renderer.DrawArrays(GL_TRIANGLES, 0, 6);
        }

        {
            overlay.Begin("Scene controls");

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                        ImGui::GetIO().Framerate);

            ImGui::Checkbox("Light volumes", &lightVolumes);

            if (ImGui::DragFloat3("Camera", &camera.eyePos[0], 0.1f)) {
                shader.Bind();
                shader.Set("camPos", camera.eyePos);

                deferredShader.Bind();
                camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);
                deferredShader.Set("view", camera.lookAt);

                volumeShader.Set("camPos", camera.eyePos);
                volumeShader.Set("view", camera.lookAt);
            }

            if (ImGui::SliderFloat3("lightPos", (float*)&lightPos[0], -10.0f, 10.0f)) {
                GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, lightUbo));
                GL_CHECK(glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::vec4), &lightPos[0]));
            }

            if (ImGui::SliderFloat("radius", (float*)&radius, 0.0f, 20.0f)) {
                shader.Bind();
                shader.Set("radius", radius);
                volumeShader.Set("radius", radius);
                volumeShader.Set("volumeScale", radius / meshInnerRadius);
            }

            float data[2]{
                metallic,
                roughness,
            };
            if (ImGui::SliderFloat2("Metallic / roughness", data, 0.0f, 1.0f)) {
                metallic  = data[0];
                roughness = data[1];
                deferredShader.Bind();
                deferredShader.Set("metallic", metallic);
                deferredShader.Set("roughness", roughness);
            }

            ImGui::Image(static_cast<ImTextureID>(gPosition), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
            ImGui::SameLine();
            ImGui::Image(static_cast<ImTextureID>(gNormal), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
            ImGui::SameLine();
            ImGui::Image(static_cast<ImTextureID>(gColorSpec), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
            ImGui::SameLine();
            ImGui::Image(static_cast<ImTextureID>(fbo_texture.GetID()), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));

            overlay.End();
        }

        context.EndFrame();
    }
}
