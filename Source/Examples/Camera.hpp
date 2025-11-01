#pragma once

#include <glm/glm.hpp>

struct Camera {
    glm::vec3 eyePos{ 0.0f, 0.0f, 5.0f };
    glm::vec3 focus{ 0.0f, 0.0f, -1.0f };
    glm::vec3 upVector{ 0.0f, 1.0f, 0.0f };

    glm::mat4 lookAt{ 1.0f };
    glm::mat4 projec{ 1.0f };

    float fov    = 0.0f;
    float aspect = 1.0f;
    float nearZ  = 0.1f;
    float farZ   = 100.0f;
};
