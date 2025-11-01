#pragma once

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <stb_image.h>

#include "Graphics/Vertex.hpp"

void CheckOpenGLErrors(const char* expression, bool pending,
                       std::source_location location = std::source_location::current());

#define ASSERT_OR_THROW(x, message)            \
    do {                                       \
        if (!(x)) {                            \
            throw std::runtime_error(message); \
        }                                      \
    } while (false)

#define GL_CHECK(x)                   \
    do {                              \
        CheckOpenGLErrors(#x, true);  \
        x;                            \
        CheckOpenGLErrors(#x, false); \
    } while (false)

#define PRINT(message) \
    std::cout << message << std::endl;

inline char* LoadFromFile(const std::string& filepath) {
    std::ifstream file{ filepath };
    std::string error = "Failed to open file: " + filepath;
    ASSERT_OR_THROW(file.is_open(), error.data());

    file.seekg(0, std::ios::end);
    int size = file.tellg();
    file.seekg(0, std::ios::beg);

    char* buffer = new char[size + 1];
    memset(buffer, 0, size + 1);
    file.read(buffer, size);
    file.close();

    return buffer;
}

inline decltype(auto) LoadTexture(const std::string& filename, bool flip) {
    std::int32_t width, height, nrChannels;
    stbi_set_flip_vertically_on_load(flip);
    auto data = stbi_load((std::string(DATA_DIRECTORY) + "Textures/" + filename).data(), &width, &height, &nrChannels, 0);
    ASSERT_OR_THROW(data, "Failed to load texture!");
    return std::tuple(data, width, height, nrChannels);
}

inline void FreeTexture(unsigned char* data) {
    stbi_image_free(data);
}

inline void LoadModel(const std::string& filename, std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices) {
    Assimp::Importer imp;
    auto pModel = imp.ReadFile(
        std::string(DATA_DIRECTORY) + "Models/" + filename,
        aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_CalcTangentSpace |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs);

    ASSERT_OR_THROW(pModel, imp.GetErrorString());
    auto pMesh = pModel->mMeshes[0];

    for (std::size_t i{}; i != pMesh->mNumVertices; ++i) {
        const auto& position = pMesh->mVertices[i];
        const auto& normal   = pMesh->mNormals[i];
        glm::vec2 uv{};
        glm::vec3 tangent{};
        glm::vec3 bitangent{};

        if (pMesh->HasTextureCoords(0)) {
            uv = { pMesh->mTextureCoords[0][i].x, pMesh->mTextureCoords[0][i].y };
        }
        if (pMesh->HasTangentsAndBitangents()) {
            const auto& t = pMesh->mTangents[i];
            const auto& b = pMesh->mBitangents[i];
            tangent       = { t.x, t.y, t.z };
            bitangent     = { b.x, b.y, b.z };
        }

        vertices.emplace_back(glm::vec3(position.x, position.y, position.z),
                              glm::vec3(normal.x, normal.y, normal.z), uv, tangent, bitangent);
    }

    for (std::size_t i{}; i != pMesh->mNumFaces; ++i) {
        const auto& face = pMesh->mFaces[i];
        indices.emplace_back(face.mIndices[0]);
        indices.emplace_back(face.mIndices[1]);
        indices.emplace_back(face.mIndices[2]);
    }
}
