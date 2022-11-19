#version 330
out vec4 fragColor;

void main()
{    
    fragColor = vec4(vec3(gl_FragDepth), 1);
}