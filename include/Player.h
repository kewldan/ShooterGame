#pragma once

#include "GameObject.h"
#include <Input.h>
#include <memory>

// Everything that describes a player at one instant, as plain data: this is what a network layer
// replicates (position/orientation/velocity for movement prediction, the rest for the HUD and the
// remote view-model). The physics body is the local authority; update() mirrors it in here.
struct PlayerState {
    glm::vec3 position{0.f};      // centre of the capsule, world space
    glm::vec3 velocity{0.f};      // world space, units per second
    float yaw = 0.f, pitch = 0.f; // radians; the camera looks along Player::getForward()
    bool grounded = false;
    bool aiming = false;          // aiming down the sights
    bool reloading = false;
    float reloadTimer = 0.f;      // seconds until the reload finishes
    int ammo = 17;                // rounds in the magazine
    int reserve = 51;             // rounds left for reloads
};

// The outcome of one raycast shot.
struct ShotResult {
    glm::vec3 origin{0.f}, direction{0.f};
    bool hit = false;
    glm::vec3 point{0.f}, normal{0.f}; // world space; the normal faces the shooter
    GameObject *object = nullptr;      // the GameObject owning the hit body, if any
    bool dynamic = false;              // the hit body moves (it received an impulse)
};

// What happened during one Player::update(); the caller turns these into sounds and effects.
struct PlayerEvents {
    bool shot = false, dryFire = false, reloadStarted = false;
    bool jumped = false, landed = false, footstep = false;
    ShotResult lastShot;
};

// The local player: a Bullet capsule driven directly by the controls (mouse look, WASD, jump with a
// raycast ground check, coyote time and reduced air control) plus the weapon logic (semi-automatic
// hitscan shots, magazine and reload). Rendering is not its business; the third-person mesh of the
// body is only drawn by the shadow and minimap passes.
class Player {
public:
    static constexpr float EYE_HEIGHT = 1.5f;      // camera above the capsule centre
    static constexpr float CAPSULE_RADIUS = 1.f;
    static constexpr float CAPSULE_HEIGHT = 2.f;   // cylinder part; the capsule is 4 units tall in total
    static constexpr float MASS = 60.f;
    static constexpr float JUMP_SPEED = 5.5f;      // units per second, upwards
    static constexpr float GROUND_ACCEL = 250.f;   // units per second^2 towards the wanted velocity
    static constexpr float AIR_CONTROL = 0.3f;     // fraction of GROUND_ACCEL available in the air
    static constexpr float COYOTE_TIME = 0.1f;     // seconds a jump is still allowed after leaving the ground
    static constexpr float GROUND_CHECK = 0.15f;   // how far below the capsule the ground may be
    static constexpr float STEP_DISTANCE = 9.f;    // units walked between two footsteps
    static constexpr int MAGAZINE = 17;
    static constexpr float RELOAD_TIME = 1.5f;
    static constexpr float FIRE_INTERVAL = 0.1f;   // seconds between two shots
    static constexpr float SHOT_RANGE = 200.f;
    static constexpr float SHOT_IMPULSE = 25.f;    // N*s given to a dynamic body that was hit
    static constexpr float SHOT_KICK = 0.004f;     // radians the view pitches up per shot

    PlayerState state;
    // Settings sliders.
    float speed = 5.f;
    float sensitivity = 1.f;
    // Keeps the sights up regardless of the mouse button (screenshots of the aiming view).
    bool forceAim = false;
    // The capsule body with the third-person mesh (player.obj).
    std::unique_ptr<GameObject> body;

    // `cameraPosition` is where the eyes start (see EYE_HEIGHT); angles in radians.
    Player(btDynamicsWorld *world, glm::vec3 cameraPosition, float yaw, float pitch);

    Player(const Player &) = delete;

    Player &operator=(const Player &) = delete;

    // Teleports the player so that the eyes are at `cameraPosition` and stops it.
    void reset(glm::vec3 cameraPosition);

    // Reads the controls and advances the state by `delta` seconds. `mouseDelta` is the cursor motion in
    // pixels; with `controlsActive` false (cursor not captured) only the physics mirror and the timers run.
    PlayerEvents update(const Engine::Input &input, glm::vec2 mouseDelta, bool controlsActive, float delta);

    // Fires one round along `direction` if the weapon can fire (ammo, reload, rate of fire); reports
    // `shot`/`dryFire` and the hit in `events`. update() calls it for the mouse, test shots call it directly.
    bool fire(glm::vec3 direction, PlayerEvents &events);

    [[nodiscard]] bool canFire() const;

    // What the player is looking at: the shot ray without a shot (nothing is pushed or consumed).
    // A debugging aid, e.g. to read world coordinates off the map.
    [[nodiscard]] ShotResult probe() const;

    [[nodiscard]] glm::vec3 getEyePosition() const;

    [[nodiscard]] glm::vec3 getForward() const;

    [[nodiscard]] glm::vec3 getRight() const;

    [[nodiscard]] float getHorizontalSpeed() const;

private:
    btDynamicsWorld *world;
    float coyoteTimer = 0.f, fireTimer = 0.f, stepDistance = 0.f;
    bool wasGrounded = false;
    float lastVerticalSpeed = 0.f;

    // Short ray from the capsule centre down past its bottom, ignoring the player's own body.
    [[nodiscard]] bool checkGrounded() const;
};
