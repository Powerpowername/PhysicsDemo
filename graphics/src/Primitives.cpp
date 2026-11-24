#include <graphics/Primitives.hpp>

#include <glm/gtc/constants.hpp>

#include <cmath>

namespace graphics::primitives
{
namespace
{
MeshData makeBox()
{
    constexpr glm::vec3 positions[][4] = {
        {{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}},
        {{1, -1, -1}, {-1, -1, -1}, {-1, 1, -1}, {1, 1, -1}},
        {{-1, -1, -1}, {-1, -1, 1}, {-1, 1, 1}, {-1, 1, -1}},
        {{1, -1, 1}, {1, -1, -1}, {1, 1, -1}, {1, 1, 1}},
        {{-1, 1, 1}, {1, 1, 1}, {1, 1, -1}, {-1, 1, -1}},
        {{-1, -1, -1}, {1, -1, -1}, {1, -1, 1}, {-1, -1, 1}},
    };
    constexpr glm::vec3 normals[] = {
        {0, 0, 1}, {0, 0, -1}, {-1, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, -1, 0},
    };

    MeshData mesh;
    mesh.vertices.reserve(24);
    mesh.indices.reserve(36);
    for (std::size_t face = 0; face < 6; ++face)
    {
        const auto first = static_cast<std::uint32_t>(mesh.vertices.size());
        for (const auto& position : positions[face])
            mesh.vertices.push_back({position, normals[face]});
        mesh.indices.insert(mesh.indices.end(), {first, first + 1, first + 2,
                                                  first, first + 2, first + 3});
    }
    return mesh;
}

MeshData makeSphere()
{
    constexpr unsigned int slices = 40;
    constexpr unsigned int stacks = 24;
    MeshData mesh;
    mesh.vertices.reserve(2 + slices * (stacks - 1));
    mesh.indices.reserve(6 * slices * (stacks - 2) + 6 * slices);
    mesh.vertices.push_back({{0, 1, 0}, {0, 1, 0}});
    mesh.vertices.push_back({{0, -1, 0}, {0, -1, 0}});
    for (unsigned int ring = 1; ring < stacks; ++ring)
    {
        const float angle = glm::pi<float>() * ring / stacks;
        const float width = std::sin(angle);
        const float height = std::cos(angle);
        for (unsigned int slice = 0; slice < slices; ++slice)
        {
            const float longitude = glm::two_pi<float>() * slice / slices;
            const glm::vec3 position{width * std::cos(longitude), height,
                                     width * std::sin(longitude)};
            mesh.vertices.push_back({position, position});
        }
    }

    for (unsigned int ring = 1; ring < stacks; ++ring)
    {
        const auto first = 2u + (ring - 1) * slices;
        for (unsigned int slice = 0; slice < slices; ++slice)
        {
            const auto left = first + slice;
            const auto right = first + (slice + 1) % slices;
            if (ring == 1)
                mesh.indices.insert(mesh.indices.end(), {0, right, left});
            else
            {
                mesh.indices.insert(mesh.indices.end(), {left - slices, right - slices, left,
                                                          right - slices, right, left});
            }
            if (ring == stacks - 1)
                mesh.indices.insert(mesh.indices.end(), {left, right, 1});
        }
    }
    return mesh;
}
}

const MeshData& box()
{
    static const MeshData mesh = makeBox();
    return mesh;
}

const MeshData& sphere()
{
    static const MeshData mesh = makeSphere();
    return mesh;
}
}
