#version 330 core
out vec4 fragColor;

in vec3 TexCoords;

uniform samplerCube skybox; // sRGB, decoded to linear by the sampler
// Linear brightness of the sky in the HDR image (the photos top out at 1).
uniform float intensity;

void main()
{
    fragColor = vec4(texture(skybox, TexCoords).rgb * intensity, 1.0);
}
