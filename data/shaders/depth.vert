#version 330 core
layout (location = 0) in vec3 vPos;

uniform mat4 lightSpaceMatrix, mvp;

void main()
{
    gl_Position = lightSpaceMatrix * mvp * vec4(vPos, 1.0);
}