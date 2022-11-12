#version 330

layout(triangles) in;
layout (triangle_strip, max_vertices=3) out;

in Vertex {
    vec3 position;
} vertex[];

out VertexData {
    vec3 normal;
    vec3 position;
} vVertexOut;

 void main()
 {

        vec3 n = cross(vertex[1].position-vertex[0].position, vertex[2].position-vertex[0].position);
        for(int i = 0; i < gl_in.length(); i++)
        {
             gl_Position = gl_in[i].gl_Position;

             vVertexOut.normal = n;
             vVertexOut.position = vertex[i].position;

             EmitVertex();
        }
}