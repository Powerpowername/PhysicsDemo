#include <graphics/SceneRenderer.hpp>
#include <graphics/Shader.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <stdexcept>
#include <cstdint>
#include <unordered_map>

namespace graphics
{
namespace
{
struct MeshResources
{
    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;
    GLuint indexBuffer = 0;
    GLsizei indexCount = 0;

    explicit MeshResources(const MeshData& mesh)
    {
        if (mesh.vertices.empty() || mesh.indices.empty() || mesh.indices.size() % 3 != 0)
            throw std::invalid_argument("SceneRenderer mesh must contain indexed triangles");
        for (const auto index : mesh.indices)
        {
            if (index >= mesh.vertices.size())
                throw std::invalid_argument("SceneRenderer mesh index is out of range");
        }
        indexCount = static_cast<GLsizei>(mesh.indices.size());
        glGenVertexArrays(1, &vertexArray);
        glGenBuffers(1, &vertexBuffer);
        glGenBuffers(1, &indexBuffer);
        glBindVertexArray(vertexArray);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(MeshVertex),
                     mesh.vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                              reinterpret_cast<const void*>(offsetof(MeshVertex, normal)));
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(std::uint32_t),
                     mesh.indices.data(), GL_STATIC_DRAW);
        glBindVertexArray(0);
    }

    MeshResources(const MeshResources&) = delete;
    MeshResources& operator=(const MeshResources&) = delete;

    ~MeshResources()
    {
        if (indexBuffer) glDeleteBuffers(1, &indexBuffer);
        if (vertexBuffer) glDeleteBuffers(1, &vertexBuffer);
        if (vertexArray) glDeleteVertexArrays(1, &vertexArray);
    }
};
}

struct SceneRenderer::Impl
{
    Shader shader{"scene/mesh.vert.glsl", "scene/mesh.frag.glsl"};
    GLint viewProjectionLocation = shader.uniformLocation("viewProjection");
    GLint modelLocation = shader.uniformLocation("model");
    GLint colorLocation = shader.uniformLocation("color");
    GLint referenceLinesLocation = shader.uniformLocation("referenceLines");
    std::unordered_map<std::uint32_t, MeshResources> meshes;
    std::uint32_t nextMesh = 1;
    glm::mat4 viewProjection{1.0f};
    GLint previousViewport[4]{};
    GLint previousScissor[4]{};
    GLboolean scissorWasEnabled = GL_FALSE;
    bool sceneOpen = false;
};

SceneRenderer::SceneRenderer() : impl_(std::make_unique<Impl>())
{
}

SceneRenderer::~SceneRenderer() = default;

void SceneRenderer::beginScene(const Camera& camera, glm::ivec2 framebufferSize)
{
    beginScene(camera, framebufferSize, {0, 0});
}

void SceneRenderer::beginScene(const Camera& camera, glm::ivec2 viewportSize, glm::ivec2 viewportOrigin)
{
    if (impl_->sceneOpen) throw std::logic_error("SceneRenderer scene is already open");
    if (viewportSize.x <= 0 || viewportSize.y <= 0)
        throw std::invalid_argument("SceneRenderer requires a positive viewport size");
    const float aspect = static_cast<float>(viewportSize.x) / viewportSize.y;
    const glm::mat4 projection = glm::perspective(glm::radians(camera.fieldOfView), aspect,
                                                   camera.nearPlane, camera.farPlane);
    const glm::mat4 view = glm::lookAt(camera.position, camera.target, camera.up);
    glEnable(GL_DEPTH_TEST);
    glGetIntegerv(GL_VIEWPORT, impl_->previousViewport);
    glGetIntegerv(GL_SCISSOR_BOX, impl_->previousScissor);
    impl_->scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST);
    glViewport(viewportOrigin.x, viewportOrigin.y, viewportSize.x, viewportSize.y);
    // glClear ignores the viewport; clip to the scene so adjacent panels are preserved.
    glEnable(GL_SCISSOR_TEST);
    glScissor(viewportOrigin.x, viewportOrigin.y, viewportSize.x, viewportSize.y);
    constexpr GLfloat background[] = {0.08f, 0.10f, 0.14f, 1.0f};
    glClearBufferfv(GL_COLOR, 0, background);
    glClear(GL_DEPTH_BUFFER_BIT);
    impl_->viewProjection = projection * view;
    impl_->sceneOpen = true;
}

MeshHandle SceneRenderer::createMesh(const MeshData& mesh)
{
    if (impl_->nextMesh == 0)
        throw std::overflow_error("SceneRenderer mesh handle space is exhausted");
    const auto id = impl_->nextMesh++;
    impl_->meshes.try_emplace(id, mesh);
    return {id};
}

void SceneRenderer::drawMesh(MeshHandle mesh, const Transform& transform,
                             const Material& material)
{
    if (!impl_->sceneOpen) throw std::logic_error("SceneRenderer scene is not open");
    const auto resource = impl_->meshes.find(mesh.value);
    if (resource == impl_->meshes.end())
        throw std::invalid_argument("SceneRenderer received an invalid mesh handle");
    const glm::mat4 model = glm::translate(glm::mat4(1.0f), transform.position)
        * glm::mat4_cast(transform.rotation)
        * glm::scale(glm::mat4(1.0f), transform.scale);
    impl_->shader.use();
    glUniformMatrix4fv(impl_->viewProjectionLocation, 1, GL_FALSE,
                       glm::value_ptr(impl_->viewProjection));
    glUniformMatrix4fv(impl_->modelLocation, 1, GL_FALSE, glm::value_ptr(model));
    glUniform4fv(impl_->colorLocation, 1, glm::value_ptr(material.color));
    glUniform1i(impl_->referenceLinesLocation, material.referenceLines ? GL_TRUE : GL_FALSE);
    glBindVertexArray(resource->second.vertexArray);
    glDrawElements(GL_TRIANGLES, resource->second.indexCount, GL_UNSIGNED_INT, nullptr);
}

void SceneRenderer::endScene()
{
    if (!impl_->sceneOpen) throw std::logic_error("SceneRenderer scene is not open");
    glBindVertexArray(0);
    glUseProgram(0);
    const auto& viewport = impl_->previousViewport;
    const auto& scissor = impl_->previousScissor;
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
    if (!impl_->scissorWasEnabled)
        glDisable(GL_SCISSOR_TEST);
    impl_->sceneOpen = false;
}
}
