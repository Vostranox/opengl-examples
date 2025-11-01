#pragma once

#include <glm/glm.hpp>

struct Vertex {
    Vertex(const glm::vec3& pos,
           const glm::vec3& normal,
           const glm::vec2& uv,
           const glm::vec3& tangent   = {},
           const glm::vec3& bitangent = {})
        : position(pos), normals(normal), texture(uv), tangent(tangent), bitangent(bitangent) {
    }

    glm::vec3 position;
    glm::vec3 normals;
    glm::vec2 texture;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};
