#version 330 core

in Vertex {
    vec3 position;
    vec3 normal;
    vec2 texCoord;
} vertex;

uniform sampler2D aTexture;
uniform int hasTexture = 0;
// 1: no lighting at all, the texture is drawn as is (muzzle flash).
uniform int unlit = 0;
// Direction TOWARDS the sun and its colour, view space (the same sun as the lighting pass).
uniform vec3 sunDir;
uniform vec3 sunColor;

out vec4 fragColor;

const float AMBIENT = 0.55;

void main()
{
    vec4 albedo = hasTexture == 1 ? texture(aTexture, vertex.texCoord) : vec4(1.0);
    if (unlit == 1) {
        fragColor = albedo;
        return;
    }
    vec3 normal = normalize(vertex.normal);
    float NdotL = max(dot(normal, sunDir), 0.0);
    // Blinn-Phong highlight: the camera sits at the origin of view space.
    vec3 viewDir = normalize(-vertex.position);
    vec3 halfway = normalize(sunDir + viewDir);
    float spec = pow(max(dot(normal, halfway), 0.0), 48.0) * 0.35;
    vec3 lighting = albedo.rgb * (AMBIENT + NdotL * sunColor) + spec * sunColor;
    fragColor = vec4(lighting, 1.0);
}
