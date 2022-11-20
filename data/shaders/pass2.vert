#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out Vertex {
    vec2 texCoord;
} vertex;

void main()
{
    vertex.texCoord = aTexCoords;
    gl_Position = vec4(aPos, 1.0);
}