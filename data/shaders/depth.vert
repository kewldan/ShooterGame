#version 330 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec2 vTexCoord;
layout (location = 2) in vec3 vNormal;

uniform mat4 lightSpaceMatrix;
uniform mat4 mvp;

void main()
{
    vec4 pos = mvp * vec4(vPos, 1.0);
    gl_Position = lightSpaceMatrix * pos;
}