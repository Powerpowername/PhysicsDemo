#pragma once

#include <graphics/Mesh.hpp>

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <memory>

namespace graphics
{
struct Transform
{
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

struct Material
{
    glm::vec4 color{1.0f};
    bool referenceLines = false;
};

struct Camera
{
    glm::vec3 position{0.0f, 3.0f, 8.0f};
    glm::vec3 target{0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    float fieldOfView = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
};

class SceneRenderer
{
public:
    SceneRenderer();
    ~SceneRenderer();

    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;

    void beginScene(const Camera& camera, glm::ivec2 framebufferSize);
    void beginScene(const Camera& camera, glm::ivec2 viewportSize, glm::ivec2 viewportOrigin);
    MeshHandle createMesh(const MeshData& mesh);
    void drawMesh(MeshHandle mesh, const Transform& transform,
                  const Material& material = {});
    void endScene();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
