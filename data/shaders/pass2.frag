#version 330 core
out vec4 FragColor;

in Vertex {
    vec2 texCoord;
} vertex;

// Lighting of the G-buffer in linear HDR (tone mapping happens later, see tonemap.frag).
// G-buffer, everything in view space (see pass1.vert). The albedo is an sRGB texture: linear here.
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D ssao;
// One layer per shadow cascade (ShadowsCaster).
uniform sampler2DArrayShadow shadowMaps;

struct Light {
    vec3 Position;
    vec3 Color;

    float Linear;
    float Quadratic;
};

const int NR_LIGHTS = 32;
uniform int nbLights;
uniform Light lights[NR_LIGHTS];

uniform int SSAO, CastShadows;

// Cascaded shadow maps.
const int NUM_CASCADES = 4;
// View space -> light clip space of each cascade (the CPU side folds inverse(view) into it).
uniform mat4 lightSpaceMats[NUM_CASCADES];
// View-space depth where each cascade ends.
uniform float cascadeSplits[NUM_CASCADES];
// World units per shadow texel of each cascade: the biases scale with it.
uniform float cascadeTexelSizes[NUM_CASCADES];
// World units spanned by the 0..1 depth of each cascade.
uniform float cascadeDepthRanges[NUM_CASCADES];
// Debug: tint the output with the cascade the pixel was shadowed from.
uniform int visualizeCascades;
// Direction TOWARDS the sun (view space) and its linear colour times intensity.
uniform vec3 sunDir;
uniform vec3 sunColor;
// Linear sky/bounce light reaching every surface (only SSAO darkens it).
uniform vec3 ambientColor;

// Blend between two cascades over this fraction of the cascade's depth range to hide the seam.
const float CASCADE_BLEND = 0.1;

// Shadow factor (0 = lit, 1 = shadowed) of `fragPos` in cascade `c`. False when the point projects
// outside the cascade's map, so the caller can fall through to the next (larger) cascade.
bool sampleCascade(int c, vec3 normal, vec3 fragPos, float NdotL, out float shadow)
{
    float texel = cascadeTexelSizes[c];
    // Push the lookup point off the surface along the normal (more where the light is grazing):
    // hides self-shadowing without the peter-panning a large depth bias causes.
    vec3 pos = fragPos + normal * texel * (1.5 + 3.0 * (1.0 - NdotL));
    vec4 fragPosLightSpace = lightSpaceMats[c] * vec4(pos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (any(lessThan(projCoords.xy, vec2(0.0))) || any(greaterThan(projCoords.xy, vec2(1.0))))
        return false;
    // Behind the far plane of the light frustum: no shadow information, treat as lit.
    if (projCoords.z > 1.0) {
        shadow = 0.0;
        return true;
    }

    // Slope-scaled depth bias, about one texel in world units, converted to the cascade's depth range.
    float bias = texel * (0.5 + 1.5 * (1.0 - NdotL)) / cascadeDepthRanges[c];
    float depth = projCoords.z - bias;

    // 3x3 PCF; the sampler compares depth in hardware and its linear filter adds a 2x2 tap.
    shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMaps, 0).xy);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            shadow += 1.0 - texture(shadowMaps, vec4(projCoords.xy + vec2(x, y) * texelSize, float(c), depth));
        }
    }
    shadow /= 9.0;
    return true;
}

// Returns the shadow factor and, in `cascade`, the cascade it came from (-1 when out of shadow range).
float ShadowCalculation(vec3 normal, vec3 fragPos, float NdotL, out int cascade)
{
    float viewDepth = -fragPos.z;
    cascade = -1;
    for (int i = 0; i < NUM_CASCADES; ++i) {
        if (viewDepth < cascadeSplits[i]) {
            cascade = i;
            break;
        }
    }
    if (cascade < 0)
        return 0.0;

    // Fall through to the next cascade when the point lies outside this one's map.
    float shadow = 0.0;
    bool found = false;
    for (int i = cascade; i < NUM_CASCADES && !found; ++i) {
        if (sampleCascade(i, normal, fragPos, NdotL, shadow)) {
            cascade = i;
            found = true;
        }
    }
    if (!found) {
        cascade = -1;
        return 0.0;
    }

    // Near the far end of the cascade, blend towards the next one.
    if (cascade < NUM_CASCADES - 1) {
        float cascadeNear = cascade > 0 ? cascadeSplits[cascade - 1] : 0.0;
        float band = CASCADE_BLEND * (cascadeSplits[cascade] - cascadeNear);
        float blendStart = cascadeSplits[cascade] - band;
        if (viewDepth > blendStart) {
            float next = 0.0;
            if (sampleCascade(cascade + 1, normal, fragPos, NdotL, next)) {
                shadow = mix(shadow, next, (viewDepth - blendStart) / band);
            }
        }
    }
    return shadow;
}

void main()
{
    vec3 FragPos = texture(gPosition, vertex.texCoord).rgb;
    vec3 Normal = normalize(texture(gNormal, vertex.texCoord).rgb);
    vec3 Diffuse = texture(gAlbedoSpec, vertex.texCoord).rgb;
    float Specular = texture(gAlbedoSpec, vertex.texCoord).a;
    float AmbientOcclusion = SSAO == 1 ? texture(ssao, vertex.texCoord).r : 1.0;

    // Ambient: never shadowed (only occluded by SSAO).
    vec3 lighting = Diffuse * ambientColor * AmbientOcclusion;

    // Sun: the only term the shadow map darkens.
    float NdotL = max(dot(Normal, sunDir), 0.0);
    float shadow = 0.0;
    int cascade = -1;
    if(CastShadows == 1 && NdotL > 0.0){
        shadow = ShadowCalculation(Normal, FragPos, NdotL, cascade);
    }
    lighting += (1.0 - shadow) * NdotL * Diffuse * sunColor;

    // FragPos is in view space, so the camera sits at the origin.
    vec3 viewDir  = normalize(-FragPos);
    for(int i = 0; i < nbLights && i < NR_LIGHTS; ++i)
    {
        // diffuse
        vec3 lightDir = normalize(lights[i].Position - FragPos);
        vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lights[i].Color;
        // specular
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
        vec3 specular = lights[i].Color * spec * Specular;
        // attenuation
        float distance = length(lights[i].Position - FragPos);
        float attenuation = 1.0 / (1.0 + lights[i].Linear * distance + lights[i].Quadratic * distance * distance);
        diffuse *= attenuation;
        specular *= attenuation;
        lighting += diffuse + specular;
    }

    if (visualizeCascades == 1 && cascade >= 0) {
        vec3 tints[NUM_CASCADES] = vec3[](vec3(1, 0.2, 0.2), vec3(0.2, 1, 0.2), vec3(0.2, 0.2, 1), vec3(1, 1, 0.2));
        lighting = mix(lighting, tints[cascade], 0.35);
    }
    FragColor = vec4(lighting, 1.0);
}
