#version 330 core

in Vertex {
    vec3 normal;
    vec2 texCoord;
} vertex;

uniform sampler2D aTexture;
uniform int hasTexture = 0;
// Direction TOWARDS the sun, world space (the same sun as the main view).
uniform vec3 sunDir;

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

    float diff = max(dot(sunDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 lighting = (ambient + diffuse) * color;

    fragColor = vec4(lighting, 1);
}
