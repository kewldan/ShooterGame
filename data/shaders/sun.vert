#version 330 core
layout (location = 0) in vec2 vCorner; // -1..1

out vec2 corner;

// A camera-facing quad in view space: `centre` far along the sun direction, `right`/`up` its half extents.
uniform mat4 proj;
uniform vec3 centre, right, up;

void main()
{
    corner = vCorner;
    gl_Position = proj * vec4(centre + right * vCorner.x + up * vCorner.y, 1.0);
}
