#version 330 core
layout (location = 0) in vec3 vPos; // unit sphere

// The light volume (or bulb) sphere: centred on the light, already in view space (see PointLights::draw).
uniform mat4 proj;
uniform vec3 position;
uniform float radius;

void main()
{
    gl_Position = proj * vec4(position + vPos * radius, 1.0);
}
