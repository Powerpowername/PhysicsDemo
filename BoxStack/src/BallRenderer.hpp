#pragma once

#include <graphics/Shader.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class BallRenderer
{
public:
    // Construct and destroy while the scene's OpenGL context is current.
    BallRenderer();
    ~BallRenderer();
    void draw(const glm::mat4& viewProjection, const glm::vec3& position,
              const glm::quat& orientation, float radius) const;

private:
    graphics::Shader shader_{"BoxStack/ball.vert.glsl", "BoxStack/ball.frag.glsl"};
    GLint viewProjectionLocation_ = shader_.uniformLocation("viewProjection");
    GLint modelLocation_ = shader_.uniformLocation("model");
    GLuint vertexArray_ = 0;
    GLuint vertexBuffer_ = 0;
    GLsizei vertexCount_ = 0;
};
