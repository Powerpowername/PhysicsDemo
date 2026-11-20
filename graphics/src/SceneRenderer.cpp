#include <graphics/SceneRenderer.hpp>
#include <graphics/Shader.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iterator>
#include <stdexcept>

namespace graphics
{
namespace
{
// The cube spans [-1, 1] on each axis, so its model scale is its half extents.
constexpr GLfloat boxVertices[][3] = {
    {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1},  { 1, 1, 1}, {-1, 1, 1}, {-1,-1, 1}, // +Z
    {-1,-1,-1}, {-1, 1,-1}, { 1, 1,-1},  { 1, 1,-1}, { 1,-1,-1}, {-1,-1,-1}, // -Z
    {-1,-1,-1}, {-1,-1, 1}, {-1, 1, 1},  {-1, 1, 1}, {-1, 1,-1}, {-1,-1,-1}, // -X
    { 1,-1,-1}, { 1, 1,-1}, { 1, 1, 1},  { 1, 1, 1}, { 1,-1, 1}, { 1,-1,-1}, // +X
    {-1, 1, 1}, { 1, 1, 1}, { 1, 1,-1},  { 1, 1,-1}, {-1, 1,-1}, {-1, 1, 1}, // +Y
    {-1,-1,-1}, { 1,-1,-1}, { 1,-1, 1},  { 1,-1, 1}, {-1,-1, 1}, {-1,-1,-1}, // -Y
};
}

struct SceneRenderer::Impl
{
    Shader shader{"scene/box.vert.glsl", "scene/box.frag.glsl"};
    GLint viewProjectionLocation = shader.uniformLocation("viewProjection");
    GLint modelLocation = shader.uniformLocation("model");
    GLint colorLocation = shader.uniformLocation("color");
    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;
    GLint previousViewport[4]{};
    GLint previousScissor[4]{};
    GLboolean scissorWasEnabled = GL_FALSE;
    bool sceneOpen = false;

    ~Impl()
    {
        if (vertexBuffer) glDeleteBuffers(1, &vertexBuffer);
        if (vertexArray) glDeleteVertexArrays(1, &vertexArray);
    }
};

SceneRenderer::SceneRenderer() : impl_(std::make_unique<Impl>())
{
    glGenVertexArrays(1, &impl_->vertexArray);
    glGenBuffers(1, &impl_->vertexBuffer);
    glBindVertexArray(impl_->vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, impl_->vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boxVertices), boxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(boxVertices[0]), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
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
    impl_->shader.use();
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
    glUniformMatrix4fv(impl_->viewProjectionLocation, 1, GL_FALSE,
                       glm::value_ptr(projection * view));
    glBindVertexArray(impl_->vertexArray);
    impl_->sceneOpen = true;
}

void SceneRenderer::drawBox(const Transform& transform, const glm::vec3& halfExtents,
                            const Material& material)
{
    if (!impl_->sceneOpen) throw std::logic_error("SceneRenderer scene is not open");
    if (halfExtents.x <= 0.0f || halfExtents.y <= 0.0f || halfExtents.z <= 0.0f)
        throw std::invalid_argument("SceneRenderer box extents must be positive");
    const glm::mat4 model = glm::translate(glm::mat4(1.0f), transform.position)
        * glm::mat4_cast(transform.rotation)
        * glm::scale(glm::mat4(1.0f), halfExtents * transform.scale);
    glUniformMatrix4fv(impl_->modelLocation, 1, GL_FALSE, glm::value_ptr(model));
    glUniform4fv(impl_->colorLocation, 1, glm::value_ptr(material.color));
    glDrawArrays(GL_TRIANGLES, 0, std::size(boxVertices));
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
