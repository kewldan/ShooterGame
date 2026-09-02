#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

// Linear HDR (+ bloom) -> display: exposure, a tone curve and the sRGB transfer function. The alpha
// channel carries the luma of the result for the FXAA pass that may follow.
uniform sampler2D hdr;
uniform sampler2D bloom; // mip 0 of the bloom chain, half resolution
uniform float exposure;
uniform float bloomStrength;
uniform int tonemapper; // 0 = ACES fitted, 1 = Uncharted 2, 2 = none

// ACES filmic curve, the fit by Stephen Hill (sRGB primaries in and out). The matrices are given column by
// column, GLSL style.
const mat3 ACES_INPUT = mat3(
    0.59719, 0.07600, 0.02840,
    0.35458, 0.90834, 0.13383,
    0.04823, 0.01566, 0.83777);
const mat3 ACES_OUTPUT = mat3(
     1.60475, -0.10208, -0.00327,
    -0.53108,  1.10813, -0.07276,
    -0.07367, -0.00605,  1.07602);

vec3 acesFitted(vec3 color)
{
    vec3 v = ACES_INPUT * color;
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return clamp(ACES_OUTPUT * (a / b), 0.0, 1.0);
}

// John Hable's curve from Uncharted 2, normalised to a white point of 11.2.
vec3 uncharted2Curve(vec3 x)
{
    const float A = 0.15, B = 0.50, C = 0.10, D = 0.20, E = 0.02, F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 uncharted2(vec3 color)
{
    const float exposureBias = 2.0;
    vec3 curved = uncharted2Curve(color * exposureBias);
    vec3 whiteScale = 1.0 / uncharted2Curve(vec3(11.2));
    return clamp(curved * whiteScale, 0.0, 1.0);
}

vec3 linearToSrgb(vec3 c)
{
    vec3 low = c * 12.92;
    vec3 high = 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055;
    // mix() with a bvec3 needs GLSL 4.5; step() does the same on 3.3.
    return mix(low, high, step(vec3(0.0031308), c));
}

void main()
{
    // The bloom chain was built from the exposed image already (bloomDown.frag).
    vec3 color = texture(hdr, TexCoords).rgb * exposure;
    color += texture(bloom, TexCoords).rgb * bloomStrength;

    if (tonemapper == 0) {
        color = acesFitted(color);
    } else if (tonemapper == 1) {
        color = uncharted2(color);
    } else {
        color = clamp(color, 0.0, 1.0);
    }

    vec3 display = linearToSrgb(color);
    FragColor = vec4(display, dot(display, vec3(0.299, 0.587, 0.114)));
}
