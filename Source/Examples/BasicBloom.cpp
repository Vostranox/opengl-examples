#include "Examples/Camera.hpp"
#include "Examples/Examples.hpp"
#include "Graphics/SceneResources.hpp"

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

namespace {
    const float bloomQuadVertices[] = { -1, 1, 0, 0, 1, -1, -1, 0, 0, 0, 1, 1, 0, 1, 1, 1, -1, 0, 1, 0 };

    struct BloomMip {
        glm::ivec2 size;
        GLuint texture;
    };

    class BloomRenderer {
      public:
        BloomRenderer(int width, int height)
            : quad(sizeof(bloomQuadVertices), bloomQuadVertices, 0, nullptr),
              downsample("bloom_downsample.glsl"),
              upsample("bloom_upsample.glsl") {
            quad.AddAttribute(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
            quad.AddAttribute(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 3 * sizeof(float));

            downsample.Bind();
            downsample.Set("srcTexture", 0);

            upsample.Bind();
            upsample.Set("srcTexture", 0);

            Resize(width, height);
        }

        void Resize(int width, int height) {
            resources.Clear();
            mips.clear();

            viewport = { width, height };
            fbo      = resources.Framebuffer();
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);

            glm::ivec2 size = viewport;
            for (int level = 0; level < 8; ++level) {
                size                 = glm::max(size / 2, glm::ivec2(1));
                const GLuint texture = resources.Texture();
                glBindTexture(GL_TEXTURE_2D, texture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, size.x, size.y, 0, GL_RGB, GL_FLOAT, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                mips.push_back({ size, texture });
                if (size.x == 1 && size.y == 1) {
                    break;
                }
            }

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mips[0].texture, 0);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                throw std::runtime_error("Bloom framebuffer is incomplete");
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        void RenderQuad() {
            quad.Bind();
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            quad.Unbind();
        }

        GLuint BloomTexture() const { return mips[0].texture; }

        void RenderBloomTexture(GLuint source, float filterRadius) {
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);

            downsample.Bind();
            downsample.Set("srcResolution", glm::vec2(viewport));
            downsample.Set("mipLevel", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, source);

            for (const auto& mip : mips) {
                glViewport(0, 0, mip.size.x, mip.size.y);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mip.texture, 0);
                RenderQuad();

                downsample.Set("srcResolution", glm::vec2(mip.size));
                downsample.Set("mipLevel", 1);
                glBindTexture(GL_TEXTURE_2D, mip.texture);
            }

            upsample.Bind();
            upsample.Set("filterRadius", filterRadius);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            glBlendEquation(GL_FUNC_ADD);

            for (int level = static_cast<int>(mips.size()) - 1; level > 0; --level) {
                const auto& sourceMip = mips[level];
                const auto& targetMip = mips[level - 1];
                glBindTexture(GL_TEXTURE_2D, sourceMip.texture);
                glViewport(0, 0, targetMip.size.x, targetMip.size.y);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, targetMip.texture, 0);
                RenderQuad();
            }

            glDisable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, viewport.x, viewport.y);
        }

      private:
        SceneResources resources;
        VertexArray quad;
        Shader downsample;
        Shader upsample;
        GLuint fbo = 0;
        glm::ivec2 viewport{};
        std::vector<BloomMip> mips;
    };
} // namespace

void RunBasicBloom(SceneContext& context) {
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

    Shader shader("basic_bloom.glsl");
    Shader emissiveShader("basic_bloom.glsl", { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER }, { "EMISSIVE" });

    Camera camera;
    camera.nearZ  = 0.1f;
    camera.farZ   = 100.0f;
    camera.aspect = float(window.GetWidth()) / window.GetHeight();
    camera.fov    = glm::radians(90.0f);
    camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);
    camera.projec = glm::perspective(camera.fov, camera.aspect, camera.nearZ, camera.farZ);

    glm::mat4 model{ 1.0f };
    glm::vec3 modelScale{ 1, 1, 1 };

    glm::vec3 lightPos{ 0, 0, 7 };
    float speculat       = 0.5f;
    float lightIntensity = 1.0f;

    float bloomFilterRadius = 0.005f;

    shader.Bind();
    shader.Set("model", model);
    shader.Set("lookAt", camera.lookAt);
    shader.Set("projection", camera.projec);
    shader.Set("viewPos", camera.eyePos);
    shader.Set("lightPos", lightPos);
    shader.Set("spec", speculat);
    shader.Set("lightIntensity", lightIntensity);

    emissiveShader.Bind();
    emissiveShader.Set("model", model);
    emissiveShader.Set("lookAt", camera.lookAt);
    emissiveShader.Set("projection", camera.projec);
    emissiveShader.Set("lightIntensity", lightIntensity);

    GLuint hdrFBO = resources.Framebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    GLuint colorBuffers[] = { resources.Texture(), resources.Texture() };
    for (unsigned int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window.GetWidth(), window.GetHeight(), 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }

    GLuint rboDepth = resources.Renderbuffer();
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window.GetWidth(), window.GetHeight());
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    BloomRenderer bloomRenderer(window.GetWidth(), window.GetHeight());

    Shader finalShader("bloom_final.glsl");
    finalShader.Bind();
    finalShader.Set("scene", 0);
    finalShader.Set("bloomBlur", 1);

    float exposure = 1;
    finalShader.Set("exposure", exposure);

    float bloomStrength = 0.04f;
    finalShader.Set("bloomStrength", bloomStrength);

    bool isemissive = false;

    while (context.BeginFrame()) {
        if (window.HasUpdated()) {
            camera.aspect = float(window.GetWidth()) / window.GetHeight();
            camera.projec = glm::perspective(camera.fov, camera.aspect, camera.nearZ, camera.farZ);

            shader.Bind();
            shader.Set("projection", camera.projec);

            emissiveShader.Bind();
            emissiveShader.Set("projection", camera.projec);

            glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
            for (unsigned int i = 0; i < 2; i++) {
                glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window.GetWidth(), window.GetHeight(), 0, GL_RGBA, GL_FLOAT,
                             NULL);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
            }

            glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window.GetWidth(), window.GetHeight());
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

            bloomRenderer.Resize(window.GetWidth(), window.GetHeight());
        }

        renderer.SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        glEnable(GL_DEPTH_TEST);
        vao.Bind();

        if (isemissive) {
            emissiveShader.Bind();
            glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
            renderer.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        } else {
            shader.Bind();
            glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
            renderer.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        }

        bloomRenderer.RenderBloomTexture(colorBuffers[1], bloomFilterRadius);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        finalShader.Bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, bloomRenderer.BloomTexture());
        bloomRenderer.RenderQuad();

        shader.Bind();

        overlay.Begin("Scene controls");

        ImGui::Checkbox("Emissive", &isemissive);

        if (ImGui::DragFloat3("Camera", &camera.eyePos[0], 0.1f)) {
            shader.Set("viewPos", camera.eyePos);

            camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);
            shader.Set("lookAt", camera.lookAt);

            emissiveShader.Bind();
            emissiveShader.Set("lookAt", camera.lookAt);
            shader.Bind();
        }

        if (ImGui::DragFloat3("Light", &lightPos[0], 0.1f)) {
            shader.Set("lightPos", lightPos);
        }

        if (ImGui::DragFloat("lightIntensity", &lightIntensity, 0.1f)) {
            emissiveShader.Bind();
            emissiveShader.Set("lightIntensity", lightIntensity);

            shader.Bind();
            shader.Set("lightIntensity", lightIntensity);
        }

        if (ImGui::DragFloat3("Scale", &modelScale[0], 0.1f)) {
            model = glm::scale(glm::mat4{ 1.0f }, modelScale);
            shader.Set("model", model);

            emissiveShader.Bind();
            emissiveShader.Set("model", model);
            shader.Bind();
        }

        ImGui::Image(static_cast<ImTextureID>(colorBuffers[0]), ImVec2(240, 240), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::SameLine();
        ImGui::Image(static_cast<ImTextureID>(colorBuffers[1]), ImVec2(240, 240), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::SameLine();
        ImGui::Image(static_cast<ImTextureID>(bloomRenderer.BloomTexture()), ImVec2(240, 240), ImVec2(0, 1),
                     ImVec2(1, 0));

        finalShader.Bind();
        if (ImGui::DragFloat("exposure", &exposure, 0.1f)) {
            finalShader.Set("exposure", exposure);
        }

        if (ImGui::DragFloat("bloomStrength", &bloomStrength, 0.1f)) {
            finalShader.Set("bloomStrength", bloomStrength);
        }

        overlay.End();

        context.EndFrame();
    }
}
