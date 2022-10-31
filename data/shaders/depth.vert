#version 330

in vec3 vPos;
in vec2 vTexCoord;
in vec3 vNormal;


uniform mat4 lightSpaceMatrix;
uniform mat4 mvp;

void main()
{
    gl_Position = lightSpaceMatrix * mvp * vec4(vPos, 1.0);
}