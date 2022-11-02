#version 330

in Vertex {
    vec3 normal;
    vec2 texCoord;
    vec3 position;
} vertex;

uniform struct Material
{
    vec3 color;
} material;

uniform struct Camera
{
    vec3 position;
    vec2 rotation;
    mat4 transform;
} camera;

uniform sampler2D aTexture;
uniform int hasTexture = 0;

out vec4 fragColor;

void main()
{
    vec3 to_light_source = normalize(camera.position - vertex.position);
    float to_dot_light = (dot(vertex.normal, to_light_source) + 1) * 0.5f;
    float diffuseFactor = to_dot_light + 0.2;

    if (hasTexture == 0){
        fragColor = vec4(material.color * diffuseFactor, 1);
    } else {
        fragColor = vec4(material.color * diffuseFactor * texture(aTexture, vertex.texCoord).xyz, 1);
    }
}