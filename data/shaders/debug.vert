#version 330

layout(location = 0) in vec3 vPos;
uniform mat4 view, proj;

out Vertex {
    vec3 position;
} vertex;

void main()
{
    gl_Position = proj * view * vec4(vPos, 1.0);
    vertex.position = vPos;
}