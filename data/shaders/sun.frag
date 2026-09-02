#version 330 core
out vec4 FragColor;

in vec2 corner; // -1..1 across the sprite; the disk fills the inner half

// The sun disk with a soft glow around it, drawn additively (the intensities are linear HDR values).
uniform vec3 color;
uniform float diskIntensity;
uniform float glowIntensity;

void main()
{
    float r = length(corner);
    float disk = 1.0 - smoothstep(0.45, 0.5, r);
    float glow = exp(-r * r * 4.0) * (1.0 - smoothstep(0.8, 1.0, r));
    FragColor = vec4(color * (disk * diskIntensity + glow * glowIntensity), 1.0);
}
