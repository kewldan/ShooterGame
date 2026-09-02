#pragma once

// A full-screen triangle strip (positions in NDC, texture coordinates 0..1) for the screen-space passes
// (lighting, post-processing, god rays). Vertex layout: location 0 = vec3 position, 1 = vec2 texCoord.
class ScreenQuad {
    unsigned int VAO = 0, VBO = 0;
public:
    ScreenQuad();

    ~ScreenQuad();

    ScreenQuad(const ScreenQuad &) = delete;

    ScreenQuad &operator=(const ScreenQuad &) = delete;

    // Draws the quad with whatever shader, framebuffer and blend state are bound.
    void draw() const;
};
