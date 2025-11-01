#include "Examples/Camera.hpp"
#include "Examples/Examples.hpp"
#include "Examples/Geometry.hpp"
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
#include "glm/ext/matrix_transform.hpp"

namespace {
    unsigned int loadCubemap(const std::vector<std::string>& faces, SceneResources& resources) {
        GLuint textureID = resources.Texture();
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

        stbi_set_flip_vertically_on_load(false);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        int width, height, nrChannels;
        for (unsigned int i = 0; i < faces.size(); i++) {
            unsigned char* data = stbi_load((std::string(DATA_DIRECTORY) + "Textures/" + "Skybox/" + faces[i]).data(),
                                            &width, &height, &nrChannels, 3);
            if (data) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                             data);
                stbi_image_free(data);
            } else {
                throw std::runtime_error("Failed to load cubemap face: " + faces[i]);
            }
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        return textureID;
    }
} // namespace

void RunBasicCubemap(SceneContext& context) {
    auto& window  = context.window;
    auto& overlay = context.overlay;
    SceneResources resources;

    std::vector<std::string> faces{ "right.jpg", "left.jpg", "top.jpg", "bottom.jpg", "front.jpg", "back.jpg" };
    unsigned int cubemapTexture = loadCubemap(faces, resources);

    bool rotate = true;

    Renderer renderer;
    renderer.Enable(GL_DEPTH_TEST);

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    LoadModel("cube.obj", vertices, indices);

    VertexArray vao(sizeof(Vertex) * vertices.size(), vertices.data(), sizeof(uint32_t) * indices.size(),
                    indices.data());
    vao.AddAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, position));
    vao.AddAttribute(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, normals));
    vao.AddAttribute(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, texture));

    VertexArray cubemapVao(sizeof(skyboxVertices), skyboxVertices, 0, nullptr);
    cubemapVao.AddAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);

    Shader shader("basic_pbr.glsl");
    Shader cubemapShader("basic_cubemap.glsl");

    Camera camera;
    camera.nearZ  = 0.1f;
    camera.farZ   = 100.0f;
    camera.aspect = float(window.GetWidth()) / window.GetHeight();
    camera.fov    = glm::radians(90.0f);
    camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);
    camera.projec = glm::perspective(camera.fov, camera.aspect, camera.nearZ, camera.farZ);

    glm::mat4 model{ 1.0f };

    cubemapShader.Bind();
    glm::mat4 view = glm::mat4(glm::mat3(camera.lookAt));
    cubemapShader.Set("view", view);
    cubemapShader.Set("projection", camera.projec);
    cubemapShader.Set("skybox", 0);

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

            cubemapShader.Bind();
            cubemapShader.Set("projection", camera.projec);
        }

        if (rotate) {
            model = glm::rotate(model, glm::radians(1.0f), { 0.0f, 1.0f, 0.0f });
            model = glm::translate(model, { 0.1f, 0.0f, 0.0f });
        }
        shader.Bind();
        shader.Set("model", model);

        renderer.SetClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        renderer.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDepthMask(GL_FALSE);
        cubemapVao.Bind();
        cubemapShader.Bind();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthMask(GL_TRUE);

        vao.Bind();
        shader.Bind();
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        overlay.Begin("Scene controls");

        if (ImGui::DragFloat3("Camera position", &camera.eyePos[0], 0.1f)) {
            camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);

            glm::mat4 view = glm::mat4(glm::mat3(camera.lookAt));
            cubemapShader.Bind();
            cubemapShader.Set("view", view);

            shader.Bind();
            shader.Set("camPos", camera.eyePos);
            shader.Set("lookAt", camera.lookAt);
        }

        if (ImGui::DragFloat3("Camera focus", &camera.focus[0], 0.1f)) {
            camera.lookAt = glm::lookAt(camera.eyePos, camera.focus, camera.upVector);

            glm::mat4 view = glm::mat4(glm::mat3(camera.lookAt));
            cubemapShader.Bind();
            cubemapShader.Set("view", view);

            shader.Bind();
            shader.Set("camPos", camera.eyePos);
            shader.Set("lookAt", camera.lookAt);
        }

        overlay.End();

        shader.Bind();
        overlay.Begin("Scene controls");

        if (ImGui::SliderFloat3("lightPos", (float*)&lightPos[0], -10.0f, 10.0f)) {
            shader.Set("lightPositions", lightPos);
        }

        if (ImGui::SliderFloat3("lightDir", (float*)&lightDir[0], -10.0f, 10.0f)) {
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
