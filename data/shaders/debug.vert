#version 330

layout(location = 0) in vec3 vPos;
uniform mat4 view, proj;

void main()
{
    gl_Position = proj * view * vec4(vPos, 1.0);
}