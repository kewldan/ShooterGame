#version 330

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in vec3 vNormal;

out Vertex {
    vec3 normal;
    vec2 texCoord;
    vec3 position;
} vertex;

uniform mat4 mvp, proj, view;

void main()
{
    vec4 mvPos = mvp * vec4(vPos, 1.0);

    gl_Position = proj * view * mvPos;

    vertex.normal = transpose(inverse(mat3(mvp))) * vNormal;
    vertex.texCoord = vTexCoord;
    vertex.position = mvPos.xyz;
}