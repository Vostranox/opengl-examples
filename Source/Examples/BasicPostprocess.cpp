#include "Examples/Camera.hpp"
#include "Examples/Examples.hpp"
#include "Examples/Geometry.hpp"

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

void RunBasicPostprocess(SceneContext& context) {
    auto& window  = context.window;
    auto& overlay = context.overlay;

    bool drawTexture = true;
    bool doPost      = true;

    Renderer renderer;

    VertexArray vao(sizeof(quadVertices2), quadVertices2, 0, nullptr);
    vao.AddAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0);
    vao.AddAttribute(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (sizeof(float) * 3));

    Texture texture(0, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, "door.tga");
    Texture texture2(1, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, "default_texture.png");

    Shader shader("default.glsl");
    shader.Bind();
    shader.Set("drawTexture", drawTexture);
    shader.Set("texture01", 0);
    shader.Set("texture02", 1);
    shader.Unbind();

    Shader shader2("post.glsl");
    shader2.Bind();
    shader2.Set("texture01", 0);
    shader2.Unbind();

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

    Texture fbo_texture(0, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR, GL_RGBA, window.GetWidth(), window.GetHeight());
    Renderbuffer rbo_depth(GL_DEPTH_COMPONENT16, window.GetWidth(), window.GetHeight());
    Framebuffer fbo;
    fbo.Bind();
    fbo.BindTexture(fbo_texture, GL_COLOR_ATTACHMENT0);
    fbo.BindRenderbuffer(rbo_depth, GL_DEPTH_ATTACHMENT);
    fbo.Unbind();

    while (context.BeginFrame()) {
        if (window.HasUpdated()) {
            camera.aspect = float(window.GetWidth()) / window.GetHeight();
            camera.projec = glm::perspective(camera.fov, camera.aspect, camera.nearZ, camera.farZ);

            shader.Bind();
            shader.Set("projection", camera.projec);

            fbo_texture.OnChangeSize(window.GetWidth(), window.GetHeight());
            rbo_depth.OnChangeSize(window.GetWidth(), window.GetHeight());
        }

        if (doPost) {
            fbo.Bind();
            renderer.Enable(GL_DEPTH_TEST);
        }
        renderer.SetClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        renderer.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        vao.Bind();
        shader.Bind();
        texture.Bind();
        texture2.Bind();

        model = glm::rotate(model, glm::radians(1.0f), { 0.0f, 1.0f, 0.0f });
        shader.Set("model", model);

        renderer.DrawArrays(GL_TRIANGLES, 0, 6);

        if (doPost) {
            fbo.Unbind();
            renderer.Disable(GL_DEPTH_TEST);
            renderer.SetClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            renderer.Clear(GL_COLOR_BUFFER_BIT);

            shader2.Bind();
            fbo_texture.Bind();

            renderer.DrawArrays(GL_TRIANGLES, 0, 6);
        }

        overlay.Begin("Scene controls");

        if (overlay.Checkbox("Draw texture", drawTexture)) {
            shader.Bind();
            shader.Set("drawTexture", drawTexture);
        }

        ImGui::Image(static_cast<ImTextureID>(fbo_texture.GetID()), ImVec2(512, 512));
        overlay.Checkbox("Do post processing", doPost);

        overlay.End();

        context.EndFrame();
    }
}
