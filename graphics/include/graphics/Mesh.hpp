#pragma once

#include <glm/vec3.hpp>

#include <cstdint>
#include <vector>

namespace graphics
{
struct MeshVertex
{
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
};

// CPU-side mesh data. SceneRenderer uploads it once when createMesh() is called.
struct MeshData
{
    std::vector<MeshVertex> vertices;
    std::vector<std::uint32_t> indices;
};

struct MeshHandle
{
    std::uint32_t value = 0;

    explicit operator bool() const { return value != 0; }
};
}
