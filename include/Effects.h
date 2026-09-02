#pragma once

#include "Shader.h"
#include <glm/glm.hpp>
#include <memory>
#include <random>
#include <vector>

// Short-lived, forward-rendered hit effects: bullet-hole decals (a ring buffer of textured quads glued
// to the hit surface, the oldest is overwritten) and tracers (camera-facing strips from the muzzle to
// the hit point that fade out). Drawn into the default framebuffer after the lighting pass, depth tested
// against the scene depth but never writing it.
class Effects {
public:
    static constexpr int MAX_DECALS = 128;
    static constexpr int MAX_TRACERS = 32;

private:
    struct Vertex {
        glm::vec3 position;
        glm::vec2 texCoord;
        glm::vec4 color;
    };

    struct Tracer {
        glm::vec3 from, to;
        float age;
    };

    std::unique_ptr<Engine::Shader> shader;
    unsigned int decalVAO = 0, decalVBO = 0, decalEBO = 0, decalTexture = 0;
    int decalNext = 0, decalCount = 0;
    unsigned int tracerVAO = 0, tracerVBO = 0;
    std::vector<Tracer> tracers;
    std::mt19937 rng{1234};

    // A radial dark spot with a soft, slightly ragged edge (RGBA8), generated once at start-up.
    static unsigned int createBulletHoleTexture();

public:
    float decalSize = 0.12f;    // edge length of a bullet hole, world units (1 unit is about 0.5 m here)
    float decalOffset = 0.005f; // lift above the surface, against z-fighting
    float tracerLife = 0.08f;   // seconds until a tracer has faded out
    float tracerWidth = 0.015f; // world units
    glm::vec3 tracerColor{1.f, 0.85f, 0.55f};

    Effects();

    ~Effects();

    Effects(const Effects &) = delete;

    Effects &operator=(const Effects &) = delete;

    // `normal` must be unit length and point away from the surface (towards the shooter).
    void addDecal(const glm::vec3 &point, const glm::vec3 &normal);

    void addTracer(const glm::vec3 &from, const glm::vec3 &to);

    void update(float delta);

    // `cameraPosition` orients the tracer strips towards the viewer.
    void draw(const glm::mat4 &proj, const glm::mat4 &view, const glm::vec3 &cameraPosition);

    [[nodiscard]] int getDecalCount() const;

    [[nodiscard]] int getTracerCount() const;
};
