#version 330 core
out vec4 FragColor;

in Vertex {
    vec2 texCoord;
} vertex;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D ssao;
uniform sampler2D shadowMap;

struct Light {
    vec3 Position;
    vec3 Color;
    
    float Linear;
    float Quadratic;
    float Radius;
};

const int NR_LIGHTS = 32;
uniform int nbLights;
uniform Light lights[NR_LIGHTS];
uniform vec3 viewPos, lightPos;
uniform int SSAO, CastShadows;
uniform mat4 lightSpaceMat;

float ShadowCalculation(vec3 in_normal, vec3 in_mvPos, vec4 in_fragPosLightSpace)
{
    vec3 projCoords = in_fragPosLightSpace.xyz / in_fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(in_normal);
    vec3 lightDir = normalize(lightPos - in_mvPos);
    //float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    // check whether current frag pos is in shadow
    float bias = 0.02;
    
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

void main()
{             
    vec3 FragPos = texture(gPosition, vertex.texCoord).rgb;
    vec3 Normal = texture(gNormal, vertex.texCoord).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, vertex.texCoord).rgb;
    float Specular = texture(gAlbedoSpec, vertex.texCoord).a;
    float AmbientOcclusion = SSAO == 1 ? texture(ssao, vertex.texCoord).r : 1;
    
    vec3 lighting  = Diffuse * 0.4 * AmbientOcclusion;
    if(CastShadows == 1){
        lighting *= 1.0 - ShadowCalculation(Normal, FragPos, lightSpaceMat * vec4(FragPos, 1));
    }
    vec3 viewDir  = normalize(viewPos - FragPos);
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