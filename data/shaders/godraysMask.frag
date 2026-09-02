#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

// Occlusion mask for the god rays, half resolution: the HDR sky and sun where the G-buffer holds no
// geometry (above `threshold`, so the plain blue sky feeds the rays far less than the sun), black where
// something blocks the view.
uniform sampler2D hdr;
uniform sampler2D gNormal;
uniform float threshold;

void main()
{
    vec3 normal = texture(gNormal, TexCoords).rgb;
    if (dot(normal, normal) > 0.001) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    vec3 light = max(texture(hdr, TexCoords).rgb - threshold, 0.0);
    FragColor = vec4(light, 1.0);
}
