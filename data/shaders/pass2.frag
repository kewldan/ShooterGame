#version 330 core
out vec4 FragColor;

in Vertex {
    vec2 texCoord;
} vertex;

// G-buffer, everything in view space (see pass1.vert).
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D ssao;
uniform sampler2DShadow shadowMap;

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
// View space -> light clip space (the CPU side folds inverse(view) into it).
uniform mat4 lightSpaceMat;
// Direction TOWARDS the sun and its colour (view space).
uniform vec3 sunDir;
uniform vec3 sunColor;

const float AMBIENT = 0.4;

float ShadowCalculation(vec3 normal, vec3 fragPos, float NdotL)
{
    // Push the lookup point off the surface along the normal (more where the light is grazing):
    // hides self-shadowing without the peter-panning a large depth bias causes.
    vec3 pos = fragPos + normal * (0.03 + 0.06 * (1.0 - NdotL));
    vec4 fragPosLightSpace = lightSpaceMat * vec4(pos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    // Behind the far plane of the light frustum: no shadow information, treat as lit.
    if (projCoords.z > 1.0)
        return 0.0;

    // Slope-scaled depth bias (depth range is 0..1 over the whole frustum depth).
    float bias = max(0.0015 * (1.0 - NdotL), 0.0003);
    float depth = projCoords.z - bias;

    // 3x3 PCF; the sampler compares depth in hardware and its linear filter adds a 2x2 tap.
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            shadow += 1.0 - texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, depth));
        }
    }
    return shadow / 9.0;
}

void main()
{             
    vec3 FragPos = texture(gPosition, vertex.texCoord).rgb;
    vec3 Normal = normalize(texture(gNormal, vertex.texCoord).rgb);
    vec3 Diffuse = texture(gAlbedoSpec, vertex.texCoord).rgb;
    float Specular = texture(gAlbedoSpec, vertex.texCoord).a;
    float AmbientOcclusion = SSAO == 1 ? texture(ssao, vertex.texCoord).r : 1.0;

    // Ambient: never shadowed (only occluded by SSAO).
    vec3 lighting = Diffuse * AMBIENT * AmbientOcclusion;

    // Sun: the only term the shadow map darkens.
    float NdotL = max(dot(Normal, sunDir), 0.0);
    float shadow = 0.0;
    if(CastShadows == 1 && NdotL > 0.0){
        shadow = ShadowCalculation(Normal, FragPos, NdotL);
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
    FragColor = vec4(lighting, 1.0);
}
