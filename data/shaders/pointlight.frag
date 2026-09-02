#version 330 core
out vec4 FragColor;

// Shades the G-buffer pixel behind a light volume with one point light (Blinn-Phong, additive onto the
// HDR image). With `emissive` set it draws the light's bulb instead: a flat, bright colour.
// G-buffer, everything in view space (see pass1.vert).
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform vec2 screenSize;

uniform vec3 position; // view space
uniform vec3 color;    // colour * intensity (linear)
uniform float radius;
uniform int emissive;

void main()
{
    if (emissive == 1) {
        FragColor = vec4(color, 1.0);
        return;
    }
    vec2 uv = gl_FragCoord.xy / screenSize;
    vec3 normal = texture(gNormal, uv).rgb;
    // Sky pixels have no geometry.
    if (dot(normal, normal) < 0.001) discard;
    normal = normalize(normal);
    vec3 fragPos = texture(gPosition, uv).rgb;

    vec3 toLight = position - fragPos;
    float distance2 = dot(toLight, toLight);
    float distance = sqrt(distance2);
    if (distance >= radius) discard;
    vec3 lightDir = toLight / distance;

    // Inverse square, windowed so that the light reaches exactly zero at its radius (Karis 2013).
    float window = clamp(1.0 - pow(distance / radius, 4.0), 0.0, 1.0);
    float attenuation = window * window / (distance2 + 1.0);

    vec4 albedoSpec = texture(gAlbedoSpec, uv);
    float NdotL = max(dot(normal, lightDir), 0.0);
    // The camera sits at the origin of view space.
    vec3 viewDir = normalize(-fragPos);
    vec3 halfway = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfway), 0.0), 32.0) * albedoSpec.a * 0.4;

    FragColor = vec4((albedoSpec.rgb * NdotL + spec) * color * attenuation, 1.0);
}
