#version 330 core

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in vec3 vNormal;

out Vertex {
    vec3 normal;
    vec2 texCoord;
} vertex;

uniform mat4 mvp; // model matrix
uniform mat4 proj, view;

void main()
{
    gl_Position = proj * view * mvp * vec4(vPos, 1.0);

    // World-space normal (the model matrix is a rigid transform, but stay correct for scaling).
    vertex.normal = transpose(inverse(mat3(mvp))) * vNormal;
    vertex.texCoord = vTexCoord;
}
