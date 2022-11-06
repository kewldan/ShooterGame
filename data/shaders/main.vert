#version 330

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in vec3 vNormal;

out Vertex {
    vec3 normal;
    vec2 texCoord;
    vec3 position;
    vec4 posLightSpace;
} vertex;

uniform struct Camera
{
    vec2 rotation;
    mat4 transform;
} camera;

uniform mat4 mvp, proj, lightSpaceMatrix;

void main()
{
    vec4 mvPos = mvp * vec4(vPos, 1.0);

    gl_Position = proj * camera.transform * mvPos;

    vertex.normal = transpose(inverse(mat3(mvp))) * vNormal;
    vertex.texCoord = vTexCoord;
    vertex.position = mvPos.xyz;
    vertex.posLightSpace = lightSpaceMatrix * mvPos;
}