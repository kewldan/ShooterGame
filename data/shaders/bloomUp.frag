#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

// One step up the bloom chain: a 3x3 tent filter of the next smaller mip, added (GL_ONE, GL_ONE blending,
// see PostProcess::renderBloom) onto the mip being upsampled into.
uniform sampler2D source;
uniform vec2 sourceTexel; // 1 / size of `source`

void main()
{
    vec2 t = sourceTexel;
    vec3 color  = texture(source, TexCoords + vec2(-1.0,  1.0) * t).rgb;
    color += 2.0 * texture(source, TexCoords + vec2( 0.0,  1.0) * t).rgb;
    color +=       texture(source, TexCoords + vec2( 1.0,  1.0) * t).rgb;
    color += 2.0 * texture(source, TexCoords + vec2(-1.0,  0.0) * t).rgb;
    color += 4.0 * texture(source, TexCoords).rgb;
    color += 2.0 * texture(source, TexCoords + vec2( 1.0,  0.0) * t).rgb;
    color +=       texture(source, TexCoords + vec2(-1.0, -1.0) * t).rgb;
    color += 2.0 * texture(source, TexCoords + vec2( 0.0, -1.0) * t).rgb;
    color +=       texture(source, TexCoords + vec2( 1.0, -1.0) * t).rgb;
    FragColor = vec4(color / 16.0, 1.0);
}
