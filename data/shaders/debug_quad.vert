#version 330
in vec3 vPos;
in vec2 vTexCoords;
in vec2 vNormal;

out vec2 TexCoords;

void main()
{
    TexCoords = vTexCoords;
    gl_Position = vec4(vPos, 1.0);
}