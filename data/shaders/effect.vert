#version 330 core

// Shared by the decals and the tracers (see Effects): world-space quads with a per-vertex tint.
layout(location = 0) in vec3 vPos;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in vec4 vColor;

out Vertex {
    vec2 texCoord;
    vec4 color;
} vertex;

uniform mat4 proj, view;

void main()
{
    gl_Position = proj * view * vec4(vPos, 1.0);
    vertex.texCoord = vTexCoord;
    vertex.color = vColor;
}
