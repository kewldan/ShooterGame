#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

// Radial blur of the occlusion mask towards the sun (Mitchell, GPU Gems 3 ch. 13): every pixel walks
// SAMPLES steps along the line to the sun, summing the mask with a geometrically decaying weight.
uniform sampler2D mask;
uniform vec2 sunPosition; // 0..1 screen space
uniform float density;    // how far towards the sun the walk goes (1 = all the way)
uniform float decay;      // per-sample weight falloff
uniform float weight;     // per-sample weight
uniform float exposure;   // overall strength (already includes the off-screen fade)

const int SAMPLES = 64; // Sun::SAMPLES

void main()
{
    vec2 delta = (TexCoords - sunPosition) * density / float(SAMPLES);
    vec2 coord = TexCoords;
    float illumination = 1.0;
    vec3 color = vec3(0.0);
    for (int i = 0; i < SAMPLES; i++) {
        coord -= delta;
        color += texture(mask, coord).rgb * illumination * weight;
        illumination *= decay;
    }
    FragColor = vec4(color * exposure, 1.0);
}
