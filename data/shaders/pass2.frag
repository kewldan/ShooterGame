#version 330 core
out vec4 FragColor;

in Vertex {
    vec2 texCoord;
} vertex;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D ssao;

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
uniform vec3 viewPos;

void main()
{             
    vec3 FragPos = texture(gPosition, vertex.texCoord).rgb;
    vec3 Normal = texture(gNormal, vertex.texCoord).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, vertex.texCoord).rgb;
    float Specular = texture(gAlbedoSpec, vertex.texCoord).a;
    float AmbientOcclusion = texture(ssao, vertex.texCoord).r;
    
    vec3 lighting  = Diffuse * 0.4 * AmbientOcclusion;
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