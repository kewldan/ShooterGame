#version 330

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in vec3 vNormal;

uniform mat4 mvp, proj, view;

void main()
{
    gl_Position = proj * view * mvp * vec4(vPos, 1.0);
}