#pragma once

#include "Camera3D.h"
#include "ScreenQuad.h"
#include "Shader.h"
#include <memory>

// The visible sun and its light shafts, both forward passes into the HDR image after the skybox:
//  - a camera-facing sprite (a hard disk with a soft glow, far brighter than 1 so it blooms) placed far
//    along the direction towards the sun and depth tested against the scene, so walls hide it;
//  - screen-space god rays (Kenny Mitchell, GPU Gems 3): an occlusion mask at half resolution (the HDR
//    sky and sun where the G-buffer has no geometry, black elsewhere), a radial blur of SAMPLES taps
//    towards the projected sun position, added back onto the HDR image. Faded out as the sun leaves
//    the screen and skipped entirely when it is behind the camera.
class Sun {
public:
    static constexpr int SAMPLES = 64; // must match godrays.frag

private:
    glm::vec3 direction; // towards the sun, world space, unit length
    glm::vec3 color;
    int w, h;            // full resolution; the ray buffers are half of it
    unsigned int spriteVAO = 0, spriteVBO = 0;
    unsigned int maskFBO = 0, maskTexture = 0, raysFBO = 0, raysTexture = 0;
    std::unique_ptr<Engine::Shader> spriteShader, maskShader, raysShader, addShader;
    ScreenQuad quad;
    // Of the last draw(): where the sun landed on screen (0..1) and how much of the rays survive.
    glm::vec2 screenPosition{0.5f};
    float fade = 0.f;

    void allocate();

public:
    // Settings.
    float angularRadius = 2.5f;   // degrees of the disk (the real sun is 0.25, this one is a game)
    float diskIntensity = 25.f;   // linear
    float glowIntensity = 3.f;
    bool godRays = true;
    float raysStrength = 0.4f;    // the "exposure" of the radial blur
    float raysDensity = 0.9f;
    float raysDecay = 0.965f;
    float raysWeight = 0.05f;
    float raysThreshold = 1.f;    // sky brighter than this (linear) feeds the rays

    // `direction` points TOWARDS the sun (world space), `color` is its linear colour (with intensity).
    Sun(const glm::vec3 &direction, const glm::vec3 &color, int width, int height);

    ~Sun();

    Sun(const Sun &) = delete;

    Sun &operator=(const Sun &) = delete;

    void resize(int width, int height);

    // Draws the sprite and the god rays into `hdrFBO` (which must be bound with the depth of the scene);
    // `hdrTexture` is its colour and `gNormal` tells sky from geometry. Leaves `hdrFBO` bound.
    void draw(Engine::Camera3D *camera, unsigned int hdrFBO, unsigned int hdrTexture, unsigned int gNormal);

    // Visibility of the sun in the last draw(): 0 = off screen or behind the camera.
    [[nodiscard]] float getFade() const;
};
