#version 410 core

layout (location = 0) in vec3 position;
uniform mat4 viewProjection;
uniform mat4 model;
out vec3 worldPosition;

void main()
{
    vec4 world = model * vec4(position, 1.0);
    worldPosition = world.xyz;
    gl_Position = viewProjection * world;
}
