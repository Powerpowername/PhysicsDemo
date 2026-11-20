#version 410 core

in vec3 localPosition;
in vec3 worldNormal;
out vec4 fragmentColor;

void main()
{
    // Paint the bands in ball coordinates so rolling changes their orientation.
    vec3 p = normalize(localPosition);
    float bandDistance = min(abs(p.y - 0.25), abs(p.x));
    float edge = max(fwidth(bandDistance), 0.001);
    float orange = smoothstep(0.05 - edge, 0.05 + edge, bandDistance);
    vec3 color = mix(vec3(0.18, 0.07, 0.03), vec3(1.0, 0.42, 0.08), orange);
    vec3 light = normalize(vec3(-0.4, 1.0, 0.6));
    float diffuse = max(dot(normalize(worldNormal), light), 0.0);
    fragmentColor = vec4(color * (0.48 + 0.52 * diffuse), 1.0);
}
