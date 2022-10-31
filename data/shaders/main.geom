#version 330

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

uniform vec2 viewportSize = vec2(800, 600);

in Vertex {
    vec3 normal;
    vec3 pos;
} vertex[];

out VertexData {
    vec3 distance;
    vec3 normal;
    vec3 pos;
} vVertexOut;

void main(void) {
    vec2 p0 = viewportSize * gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;
    vec2 p1 = viewportSize * gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w;
    vec2 p2 = viewportSize * gl_in[2].gl_Position.xy / gl_in[2].gl_Position.w;

    vec2 v0 = p2 - p1;
    vec2 v1 = p2 - p0;
    vec2 v2 = p1 - p0;
    float fArea = abs(v1.x * v2.y - v1.y * v2.x);

    vVertexOut.distance = vec3(fArea/length(v0), 0, 0);
    vVertexOut.normal = vertex[0].normal;
    vVertexOut.pos = vertex[0].pos;
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    vVertexOut.distance = vec3(0, fArea/length(v1), 0);
    vVertexOut.normal = vertex[1].normal;
    vVertexOut.pos = vertex[1].pos;
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    vVertexOut.distance = vec3(0, 0, fArea/length(v2));
    vVertexOut.normal = vertex[2].normal;
    vVertexOut.pos = vertex[2].pos;
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
}