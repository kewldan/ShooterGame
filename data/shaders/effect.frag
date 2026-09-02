#version 330 core

in Vertex {
    vec2 texCoord;
    vec4 color;
} vertex;

uniform sampler2D aTexture;
uniform int hasTexture = 0;

out vec4 fragColor;

void main()
{
    fragColor = vertex.color * (hasTexture == 1 ? texture(aTexture, vertex.texCoord) : vec4(1.0));
}
