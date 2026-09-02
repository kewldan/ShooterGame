#version 330 core

// Depth-only pass: the bias lives in glPolygonOffset (ShadowsCaster::pass) and pass2.frag,
// writing gl_FragDepth here would only disable early depth testing.
void main()
{
}
