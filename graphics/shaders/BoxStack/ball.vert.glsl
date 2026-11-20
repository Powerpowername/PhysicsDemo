#version 410 core

layout (location = 0) in vec3 position;
uniform mat4 model;
uniform mat4 viewProjection;
out vec3 localPosition;
out vec3 worldNormal;

void main()
{
    localPosition = position;
    // A unit sphere's local normal is its position; the model scale is uniform.
    worldNormal = mat3(model) * position;
    gl_Position = viewProjection * model * vec4(position, 1.0);
}
