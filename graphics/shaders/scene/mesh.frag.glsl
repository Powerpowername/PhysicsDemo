#version 410 core

in vec3 localPosition;
in vec3 worldNormal;
uniform vec4 color;
uniform bool referenceLines;
out vec4 fragmentColor;

void main()
{
    vec3 surfaceColor = color.rgb;
    if (referenceLines)
    {
        // The pattern is evaluated in object space, so it rotates with the mesh.
        vec3 p = normalize(localPosition);
        float lineDistance = min(abs(p.y - 0.25), abs(p.x));
        float edge = max(fwidth(lineDistance), 0.001);
        float line = smoothstep(0.05 - edge, 0.05 + edge, lineDistance);
        surfaceColor = mix(vec3(0.18, 0.07, 0.03), surfaceColor, line);
    }
    vec3 normal = normalize(worldNormal);
    vec3 lightDirection = normalize(vec3(-0.4, 1.0, 0.6));
    float diffuse = max(dot(normal, lightDirection), 0.0);
    fragmentColor = vec4(surfaceColor * (0.55 + 0.45 * diffuse), color.a);
}
