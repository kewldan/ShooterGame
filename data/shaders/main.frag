#version 330

in VertexData {
    vec3 distance;
    vec3 normal;
    vec3 position;
    vec2 texCoord;
    vec4 posLightSpace;
} vertex;

uniform struct Camera
{
    vec2 rotation;
    mat4 transform;
} camera;

uniform struct Environment
{
    vec3 sun_position;
} environment;

uniform sampler2D shadowMap;
uniform sampler2D aTexture;
uniform int hasTexture = 0;
uniform int displayWireframe = 0;
uniform int castShadows = 1;

out vec4 fragColor;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(vertex.normal);
    vec3 lightDir = normalize(environment.sun_position - vertex.position);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), -0.005);  

    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 0.5 / textureSize(shadowMap, 0);
    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if (projCoords.z > 1.0)
    shadow = 0.0;

    return shadow;
}

void main()
{
    vec3 color = vec3(1);
    if (hasTexture == 1){
        color = texture(aTexture, vertex.texCoord).xyz;
    }
    vec3 normal = normalize(vertex.normal);
    vec3 lightColor = vec3(1.0);

    //ambient
    vec3 ambient = 0.15 * lightColor;

    //sun
    vec3 lightDir = normalize(environment.sun_position - vertex.position);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;

    // calculate shadow
    vec3 lighting;
    if(castShadows == 1){
        float shadow = ShadowCalculation(vertex.posLightSpace);
        lighting = (ambient + (1.0 - shadow) * diffuse) * color;
    }else{
        lighting = (ambient + diffuse) * color;
    }

    float fNearest = min(min(vertex.distance[0], vertex.distance[1]), vertex.distance[2]);
    float fEdgeIntensity = clamp(exp2(-0.1 * fNearest * fNearest), 0.0, 1.0);

    fragColor = vec4((fEdgeIntensity > 0.97 && displayWireframe == 1) ? vec3(0) : lighting, 1);
}