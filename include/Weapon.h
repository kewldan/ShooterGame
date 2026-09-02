#pragma once

#include "GameObject.h"
#include "Shader.h"
#include <memory>
#include <vector>

// The first-person view-model: the equipped weapon drawn in its own forward pass on top of the lit scene.
// It is defined in view space (so it follows the camera by construction), uses its own projection (a
// narrower FOV and a tiny near plane) and a cleared depth buffer, so it never cuts into walls. It takes
// no part in the shadow, minimap or G-buffer passes. All motion (sway, bob, recoil, aiming) is purely
// cosmetic and lives here; the gameplay state (ammo, aiming) belongs to Player.
class Weapon {
    std::vector<Mesh> meshes;
    std::vector<GameObject::Chunk> chunks;
    std::unique_ptr<Engine::Shader> shader;
    Mesh flashQuad;
    unsigned int flashTexture = 0;

    // Animated state, see update().
    glm::vec2 sway{0.f};   // lag behind the mouse (yaw, pitch), radians
    glm::vec3 bob{0.f};    // walking oscillation, view space
    float bobPhase = 0.f, bobBlend = 0.f;
    float recoil = 0.f;    // 1 right after a shot, decays to 0
    float aim = 0.f;       // 0 = hip, 1 = aiming down the sights (smoothed)
    float flashTimer = 0.f, flashSpin = 0.f;
    glm::mat4 model{1.f};  // view space, as last computed by update()

    // A bright radial core with four soft spikes (RGB, drawn additively).
    static unsigned int createFlashTexture();

public:
    // Tunables (the settings window exposes them).
    float fov = 55.f;                           // horizontal, degrees
    float scale = 0.18f;                        // the mesh is about 2 units long
    glm::vec3 hipOffset{0.14f, -0.1f, -0.42f};  // view space: right, up, forward (-z)
    glm::vec3 aimOffset{0.f, -0.067f, -0.4f};   // brings the sights to the screen centre
    glm::vec3 baseRotation{0.f};                // degrees, corrects the mesh's own orientation
    glm::vec3 muzzleLocal{0.f, 0.3f, -1.16f};   // mesh space
    float swayAmount = 0.00006f;                // radians per (pixel / s) of mouse speed
    float swayMax = 0.05f, swaySpeed = 10.f;
    float bobAmount = 0.012f, bobFrequency = 1.8f;
    float recoilKick = 0.06f;                   // push towards the camera, view units
    float recoilAngle = 7.f;                    // muzzle rise, degrees
    float recoilRecovery = 12.f;                // 1/s
    float flashSize = 0.09f, flashDuration = 0.035f;

    explicit Weapon(const char *meshPath);

    ~Weapon();

    Weapon(const Weapon &) = delete;

    Weapon &operator=(const Weapon &) = delete;

    // Call once per shot: starts the recoil kick and the muzzle flash.
    void kick();

    // `mouseDelta` in pixels this frame, `speed` the horizontal speed of the player (drives the bob).
    void update(glm::vec2 mouseDelta, float speed, bool grounded, bool aiming, float delta);

    // Muzzle position in world space, for the tracers. The view-model has its own projection, so the
    // point is first moved to where it appears on screen under the main camera's projection (`proj`,
    // with `aspect`), then taken back to world space with `inverseView`.
    [[nodiscard]] glm::vec3 getMuzzleWorld(const glm::mat4 &proj, const glm::mat4 &inverseView, float aspect) const;

    // Draws the weapon and the muzzle flash. `sunDir` points TOWARDS the sun, in view space.
    void draw(float aspect, const glm::vec3 &sunDir, const glm::vec3 &sunColor);

    [[nodiscard]] float getAim() const;
};
