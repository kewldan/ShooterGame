#version 330

in vec3 vPos;
in vec2 vTexCoord;
in vec3 vNormal;

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