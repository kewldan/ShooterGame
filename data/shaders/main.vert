#version 330

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in vec3 vNormal;

out Vertex {
    vec3 normal;
    vec3 position;
} vertex;

uniform struct Camera
{
    vec3 position;
    vec2 rotation;
    mat4 transform;
} camera;

uniform mat4 mvp, proj;

void main()
{
    vec4 mvPos = mvp * vec4(vPos, 1.0);

    gl_Position = proj * camera.transform * mvPos;

    vertex.normal = vNormal;
    vertex.position = mvPos.xyz;
}