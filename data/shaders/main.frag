#version 330

in Vertex {
    vec3 normal;
    vec3 position;
} vertex;

uniform struct Material
{
    vec3 color;
} material;

uniform struct Camera
{
    vec3 position;
    vec2 rotation;
    mat4 transform;
} camera;

out vec4 fragColor;

void main()
{
    vec3 to_light_source = normalize(camera.position - vertex.position);
    float to_dot_light = (dot(vertex.normal, to_light_source) + 1) * 0.5f;
    float diffuseFactor = to_dot_light + 0.1;

    fragColor = vec4(material.color * diffuseFactor, 1);
}