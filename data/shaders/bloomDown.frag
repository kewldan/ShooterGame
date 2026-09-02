#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

// One step down the bloom chain: the 13-tap downsample of Jimenez (Call of Duty: Advanced Warfare),
// which halves the resolution without the flickering a plain box filter gives. The first step
// (`prefilter`) reads the HDR image and keeps only what lies above the threshold, with a soft knee so the
// bloom does not pop in and out on surfaces hovering around it.
uniform sampler2D source;
uniform vec2 sourceTexel; // 1 / size of `source`
uniform int prefilter;
uniform float exposure;  // the threshold applies to the exposed image, like the eye sees it
uniform float threshold;
uniform float knee;

vec3 applyThreshold(vec3 color)
{
    float brightness = max(color.r, max(color.g, color.b));
    float soft = clamp(brightness - threshold + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 1e-4);
    float contribution = max(soft, brightness - threshold) / max(brightness, 1e-4);
    return color * contribution;
}

void main()
{
    vec2 t = sourceTexel;
    vec3 a = texture(source, TexCoords + vec2(-2.0,  2.0) * t).rgb;
    vec3 b = texture(source, TexCoords + vec2( 0.0,  2.0) * t).rgb;
    vec3 c = texture(source, TexCoords + vec2( 2.0,  2.0) * t).rgb;
    vec3 d = texture(source, TexCoords + vec2(-2.0,  0.0) * t).rgb;
    vec3 e = texture(source, TexCoords).rgb;
    vec3 f = texture(source, TexCoords + vec2( 2.0,  0.0) * t).rgb;
    vec3 g = texture(source, TexCoords + vec2(-2.0, -2.0) * t).rgb;
    vec3 h = texture(source, TexCoords + vec2( 0.0, -2.0) * t).rgb;
    vec3 i = texture(source, TexCoords + vec2( 2.0, -2.0) * t).rgb;
    vec3 j = texture(source, TexCoords + vec2(-1.0,  1.0) * t).rgb;
    vec3 k = texture(source, TexCoords + vec2( 1.0,  1.0) * t).rgb;
    vec3 l = texture(source, TexCoords + vec2(-1.0, -1.0) * t).rgb;
    vec3 m = texture(source, TexCoords + vec2( 1.0, -1.0) * t).rgb;

    vec3 color;
    if (prefilter == 1) {
        // Threshold the five overlapping 2x2 groups separately (partial Karis average): a single very
        // bright pixel then cannot swamp its whole neighbourhood.
        float e4 = exposure * 0.25;
        color  = applyThreshold((j + k + l + m) * e4) * 0.5;
        color += applyThreshold((a + b + d + e) * e4) * 0.125;
        color += applyThreshold((b + c + e + f) * e4) * 0.125;
        color += applyThreshold((d + e + g + h) * e4) * 0.125;
        color += applyThreshold((e + f + h + i) * e4) * 0.125;
    } else {
        color  = e * 0.125;
        color += (a + c + g + i) * 0.03125;
        color += (b + d + f + h) * 0.0625;
        color += (j + k + l + m) * 0.125;
    }
    FragColor = vec4(color, 1.0);
}
