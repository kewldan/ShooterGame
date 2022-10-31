#version 330

in vec3 vPos;
in vec2 vTexCoord;
in vec3 vNormal;

out Vertex {
    vec3 normal;
    vec3 pos;
} vertex;

uniform mat4 proj, mvp;

void main()
{
    vec4 mvPos = mvp * vec4(vPos, 1.0);

    gl_Position = proj * mvPos;

    vertex.normal = vNormal;
    vertex.pos = mvPos.xyz;
}