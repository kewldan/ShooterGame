#version 330 core

in Vertex {
    vec3 position;
    vec3 normal;
    vec2 texCoord;
} vertex;

uniform sampler2D aTexture; // sRGB encoded, decoded below
uniform int hasTexture = 0;
// 1: no lighting at all, the texture is drawn as is, scaled by `emissive` (muzzle flash).
uniform int unlit = 0;
uniform float emissive = 1.0;
// Direction TOWARDS the sun, view space, and the linear sun and ambient colours of the lighting pass,
// so the weapon sits in the same light as the scene (the HDR image is tone mapped later).
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec3 ambientColor;

out vec4 fragColor;

void main()
{
    vec4 albedo = vec4(1.0);
    if (hasTexture == 1) {
        albedo = texture(aTexture, vertex.texCoord);
        albedo.rgb = pow(albedo.rgb, vec3(2.2));
    }
    if (unlit == 1) {
        fragColor = vec4(albedo.rgb * emissive, albedo.a);
        return;
    }
    vec3 normal = normalize(vertex.normal);
    float NdotL = max(dot(normal, sunDir), 0.0);
    // Blinn-Phong highlight: the camera sits at the origin of view space.
    vec3 viewDir = normalize(-vertex.position);
    vec3 halfway = normalize(sunDir + viewDir);
    float spec = pow(max(dot(normal, halfway), 0.0), 48.0) * 0.35;
    vec3 lighting = albedo.rgb * (ambientColor + NdotL * sunColor) + spec * sunColor;
    fragColor = vec4(lighting, 1.0);
}
