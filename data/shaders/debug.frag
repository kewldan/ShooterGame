#version 330

in VertexData {
    vec3 normal;
    vec3 position;
} vertex;

out vec4 fragColor;

void main()
{
    float dt = dot(vec3(0.2, 1, 0.2),vertex.normal);
    float diffuse = max((dt + 1) * 0.5, 0.2);
    vec3 color = diffuse * vec3(1, 0.1, 0.2);

    fragColor = vec4(color, 1);
}