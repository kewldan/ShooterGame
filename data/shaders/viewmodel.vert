#version 330 core

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in vec3 vNormal;

out Vertex {
    vec3 position; // view space
    vec3 normal;   // view space
    vec2 texCoord;
} vertex;

// The view-model lives in view space: `model` places it relative to the camera and `proj` is its own
// perspective (narrower FOV and a tiny near plane, see Weapon::draw).
uniform mat4 model;
uniform mat4 proj;

void main()
{
    vec4 viewPos = model * vec4(vPos, 1.0);
    vertex.position = viewPos.xyz;
    vertex.normal = transpose(inverse(mat3(model))) * vNormal;
    vertex.texCoord = vTexCoord;
    gl_Position = proj * viewPos;
}
