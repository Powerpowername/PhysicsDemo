#include "BallRenderer.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <vector>

namespace
{
constexpr int slices = 40;
constexpr int stacks = 24;

glm::vec3 spherePoint(int ring, int slice)
{
    const float latitude = glm::pi<float>() * ring / stacks;
    const float longitude = glm::two_pi<float>() * slice / slices;
    return {std::sin(latitude) * std::cos(longitude), std::cos(latitude),
            std::sin(latitude) * std::sin(longitude)};
}
}

BallRenderer::BallRenderer()
{
    std::vector<glm::vec3> vertices;
    vertices.reserve(6 * slices * (stacks - 1));
    for (int ring = 0; ring < stacks; ++ring)
    {
        for (int slice = 0; slice < slices; ++slice)
        {
            const auto a = spherePoint(ring, slice);
            const auto b = spherePoint(ring, slice + 1);
            const auto c = spherePoint(ring + 1, slice);
            const auto d = spherePoint(ring + 1, slice + 1);
            // At each pole one edge collapses to a point: emit just one triangle.
            if (ring > 0)          vertices.insert(vertices.end(), {a, b, c});
            if (ring < stacks - 1) vertices.insert(vertices.end(), {b, d, c});
        }
    }
    vertexCount_ = static_cast<GLsizei>(vertices.size());
    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);
    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER, vertexCount_ * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

BallRenderer::~BallRenderer()
{
    glDeleteBuffers(1, &vertexBuffer_);
    glDeleteVertexArrays(1, &vertexArray_);
}

void BallRenderer::draw(const glm::mat4& viewProjection, const glm::vec3& position,
                        const glm::quat& orientation, float radius) const
{
    const glm::mat4 pose = glm::translate(glm::mat4{1.0f}, position) * glm::mat4_cast(orientation);
    const glm::mat4 model = glm::scale(pose, glm::vec3{radius});
    shader_.use();
    glUniformMatrix4fv(viewProjectionLocation_, 1, GL_FALSE, glm::value_ptr(viewProjection));
    glUniformMatrix4fv(modelLocation_, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(vertexArray_);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount_);
}
