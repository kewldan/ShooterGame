#version 330

in Vertex {
    vec3 normal;
    vec2 texCoord;
    vec3 position;
} vertex;

uniform sampler2D aTexture;
uniform int hasTexture = 0;

out vec4 fragColor;

void main()
{
    vec3 color = vec3(1);
    if (hasTexture == 1){
        color = texture(aTexture, vertex.texCoord).xyz;
    }
    vec3 normal = normalize(vertex.normal);
    vec3 lightColor = vec3(1.0);

    vec3 ambient = 0.15 * lightColor;

    vec3 lightDir = normalize(vec3(-3.5, 10, -1.5) - vertex.position);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 lighting = (ambient + diffuse) * color;

    fragColor = vec4(lighting, 1);
}