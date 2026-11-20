#version 410 core

in vec3 worldPosition;
uniform vec4 color;
out vec4 fragmentColor;

void main()
{
    // Adjacent fragments on a flat box face share the same surface normal.
    vec3 normal = normalize(cross(dFdx(worldPosition), dFdy(worldPosition)));
    vec3 lightDirection = normalize(vec3(-0.4, 1.0, 0.6));
    float diffuse = max(dot(normal, lightDirection), 0.0);
    fragmentColor = vec4(color.rgb * (0.55 + 0.45 * diffuse), color.a);
}
