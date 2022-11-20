#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;
in vec2 TexCoords;
in vec3 Normal;

uniform sampler2D aTexture;
uniform int hasTexture = 0;

void main()
{    
    gPosition = FragPos;
    gNormal = normalize(Normal);
    if(hasTexture == 1){
        gAlbedoSpec.rgb = texture(aTexture, TexCoords).rgb;
    }else{
        gAlbedoSpec.rgb = vec3(1);
    }
}