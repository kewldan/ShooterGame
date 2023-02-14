#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;

uniform mat4 mvp; //Model matrix
uniform mat4 view; //View (camera transformations)
uniform mat4 proj; //Projection (Only for screen)

void main()
{
    vec4 worldPos = view * mvp * vec4(aPos, 1.0);
    FragPos = worldPos.xyz; 
    TexCoords = aTexCoords;
    
    mat3 normalMatrix = transpose(inverse(mat3(view * mvp)));
    Normal = normalMatrix * aNormal;

    gl_Position = proj * worldPos;
}