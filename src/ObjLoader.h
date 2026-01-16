#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;
    glm::vec3 nrm;
};

struct Material {
    std::string name;
    glm::vec3 Kd{1,1,1};
    glm::vec3 Ks{0.2f,0.2f,0.2f};
    float Ns{32.f};
    std::string mapKd;     // pełna ścieżka do tekstury
    unsigned int glTex = 0; // uchwyt GL po załadowaniu
};

struct SubMesh {
    std::string materialName;
    uint32_t indexOffset = 0;
    uint32_t indexCount = 0;
};

struct LoadedModel {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<SubMesh> submeshes;
    std::unordered_map<std::string, Material> materials;
};

LoadedModel LoadOBJ_WithMTL(const std::string& objPath, const std::string& baseDir);
