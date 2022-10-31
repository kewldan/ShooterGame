#version 330

in VertexData {
    vec3 distance;
    vec3 normal;
    vec3 pos;
} vertex;

vec3 diffuseColor = vec3(1, 0, 1);
vec3 lightPosition = vec3(3, 3, 3);
float wireframe = 1;

out vec4 fragColor;

void main()
{
    float fNearest = min(min(vertex.distance[0], vertex.distance[1]), vertex.distance[2]);
    float fEdgeIntensity = clamp(exp2(-0.1 * fNearest * fNearest), 0.0, 1.0);

    vec3 to_light_source = normalize(lightPosition - vertex.pos);
    float to_dot_light = dot(vertex.normal, to_light_source);
    float diffuseFactor = to_dot_light;

    float frame = min(fEdgeIntensity, wireframe);

    fragColor = vec4(mix(diffuseColor * diffuseFactor, vec3(0.2), frame), 1);
}