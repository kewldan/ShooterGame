#include "Player.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>

namespace {
    // Closest hit that skips one body (the player's own capsule, which every ray starts inside of).
    struct RayCallback : btCollisionWorld::ClosestRayResultCallback {
        const btCollisionObject *ignored;

        RayCallback(const btVector3 &from, const btVector3 &to, const btCollisionObject *ignored)
                : ClosestRayResultCallback(from, to), ignored(ignored) {
        }

        bool needsCollision(btBroadphaseProxy *proxy) const override {
            return proxy->m_clientObject != ignored && ClosestRayResultCallback::needsCollision(proxy);
        }
    };

    btVector3 toBullet(const glm::vec3 &v) {
        return {v.x, v.y, v.z};
    }

    glm::vec3 toGlm(const btVector3 &v) {
        return {v.x(), v.y(), v.z()};
    }

    // Moves `value` towards `target` by at most `step`, per component.
    glm::vec2 moveTowards(glm::vec2 value, glm::vec2 target, float step) {
        const glm::vec2 diff = target - value;
        const float distance = glm::length(diff);
        if (distance <= step || distance < 1e-6f) {
            return target;
        }
        return value + diff * (step / distance);
    }
}

Player::Player(btDynamicsWorld *world, glm::vec3 cameraPosition, float yaw, float pitch) : world(world) {
    body = std::make_unique<GameObject>(world, "player.obj", MASS, new btCapsuleShape(CAPSULE_RADIUS, CAPSULE_HEIGHT));
    body->rb->setAngularFactor(0.f);
    body->rb->setSleepingThresholds(0.f, 0.f);
    // The capsule slides along walls and floors on its own velocity; Bullet's friction would only fight it.
    body->rb->setFriction(0.f);
    body->setCastShadows(true);
    state.yaw = yaw;
    state.pitch = pitch;
    reset(cameraPosition);
}

void Player::reset(glm::vec3 cameraPosition) {
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(toBullet(cameraPosition - glm::vec3(0.f, EYE_HEIGHT, 0.f)));
    body->rb->setWorldTransform(transform);
    body->motionState->setWorldTransform(transform);
    body->rb->setLinearVelocity(btVector3(0.f, 0.f, 0.f));
    body->rb->setAngularVelocity(btVector3(0.f, 0.f, 0.f));
    body->rb->clearForces();
    state.position = toGlm(transform.getOrigin());
    state.velocity = glm::vec3(0.f);
    state.grounded = false;
    coyoteTimer = 0.f;
}

bool Player::checkGrounded() const {
    const btVector3 from = body->rb->getWorldTransform().getOrigin();
    const float halfHeight = CAPSULE_RADIUS + CAPSULE_HEIGHT * 0.5f;
    const btVector3 to = from - btVector3(0.f, halfHeight + GROUND_CHECK, 0.f);
    RayCallback callback(from, to, body->rb.get());
    world->rayTest(from, to, callback);
    return callback.hasHit();
}

PlayerEvents Player::update(const Engine::Input &input, glm::vec2 mouseDelta, bool controlsActive, float delta) {
    PlayerEvents events;
    delta = std::clamp(delta, 0.f, 0.1f);

    // Mirror the physics body.
    btRigidBody &rb = *body->rb;
    state.position = toGlm(rb.getWorldTransform().getOrigin());
    state.velocity = toGlm(rb.getLinearVelocity());
    state.grounded = checkGrounded();
    if (state.grounded) {
        coyoteTimer = COYOTE_TIME;
        if (!wasGrounded && lastVerticalSpeed < -2.f) {
            events.landed = true;
        }
    } else {
        coyoteTimer = std::max(coyoteTimer - delta, 0.f);
    }
    wasGrounded = state.grounded;
    lastVerticalSpeed = state.velocity.y;

    // Look.
    if (controlsActive) {
        state.yaw += mouseDelta.x * 0.001f * sensitivity;
        state.pitch += mouseDelta.y * 0.001f * sensitivity;
        state.pitch = std::clamp(state.pitch, -1.5f, 1.5f);
        if (state.yaw >= glm::two_pi<float>()) state.yaw -= glm::two_pi<float>();
        if (state.yaw <= -glm::two_pi<float>()) state.yaw += glm::two_pi<float>();
    }

    // Walk: the wanted horizontal velocity from WASD, approached quickly on the ground and slowly in the air.
    glm::vec2 wish(0.f);
    if (controlsActive && alive) {
        const float slowWalk = input.isKeyPressed(GLFW_KEY_LEFT_SHIFT) ? 0.8f : 1.5f;
        const float moveSpeed = 5.f * speed * slowWalk;
        const glm::vec3 forward = getForward(), right = getRight();
        const glm::vec2 flatForward = glm::normalize(glm::vec2(forward.x, forward.z));
        const glm::vec2 flatRight(right.x, right.z);
        if (input.isKeyPressed(GLFW_KEY_W)) wish += flatForward;
        else if (input.isKeyPressed(GLFW_KEY_S)) wish -= flatForward;
        if (input.isKeyPressed(GLFW_KEY_D)) wish += flatRight;
        else if (input.isKeyPressed(GLFW_KEY_A)) wish -= flatRight;
        if (glm::dot(wish, wish) > 0.f) {
            wish = glm::normalize(wish) * moveSpeed;
        }
    }
    const float accel = GROUND_ACCEL * (state.grounded ? 1.f : AIR_CONTROL);
    const glm::vec2 horizontal = moveTowards(glm::vec2(state.velocity.x, state.velocity.z), wish, accel * delta);
    state.velocity.x = horizontal.x;
    state.velocity.z = horizontal.y;

    // Jump: only from the ground (or just after leaving it); a set vertical speed, not an impulse, so
    // it is the same whether the body was falling a little or not.
    if (controlsActive && alive && input.isKeyJustPressed(GLFW_KEY_SPACE) && coyoteTimer > 0.f) {
        state.velocity.y = JUMP_SPEED;
        coyoteTimer = 0.f;
        state.grounded = false;
        events.jumped = true;
    }
    rb.setLinearVelocity(toBullet(state.velocity));
    rb.activate(true);

    // Footsteps: by distance walked on the ground, so they follow the pace.
    if (state.grounded && getHorizontalSpeed() > 1.f) {
        stepDistance += getHorizontalSpeed() * delta;
        if (stepDistance >= STEP_DISTANCE) {
            stepDistance = 0.f;
            events.footstep = true;
        }
    } else {
        stepDistance = STEP_DISTANCE * 0.5f; // the first step after landing comes soon
    }

    // Weapon: reload, rate of fire, aiming, and the trigger.
    fireTimer = std::max(fireTimer - delta, 0.f);
    if (state.reloading) {
        state.reloadTimer -= delta;
        if (state.reloadTimer <= 0.f) {
            const int rounds = std::min(MAGAZINE - state.ammo, state.reserve);
            state.ammo += rounds;
            state.reserve -= rounds;
            state.reloading = false;
            state.reloadTimer = 0.f;
        }
    } else if (controlsActive && input.isKeyJustPressed(GLFW_KEY_R) && state.ammo < MAGAZINE && state.reserve > 0) {
        state.reloading = true;
        state.reloadTimer = RELOAD_TIME;
        events.reloadStarted = true;
    }
    state.aiming = forceAim || (controlsActive && input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT));
    if (controlsActive && input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        fire(getForward(), events);
    }
    return events;
}

bool Player::canFire() const {
    return alive && !state.reloading && fireTimer <= 0.f;
}

bool Player::fire(glm::vec3 direction, PlayerEvents &events) {
    if (!canFire()) {
        return false;
    }
    if (state.ammo <= 0) {
        events.dryFire = true;
        fireTimer = FIRE_INTERVAL;
        return false;
    }
    state.ammo--;
    fireTimer = FIRE_INTERVAL;
    state.pitch = std::max(state.pitch - SHOT_KICK, -1.5f);

    ShotResult &shot = events.lastShot;
    shot.origin = getEyePosition();
    shot.direction = glm::normalize(direction);
    const btVector3 from = toBullet(shot.origin), to = toBullet(shot.origin + shot.direction * SHOT_RANGE);
    RayCallback callback(from, to, body->rb.get());
    world->rayTest(from, to, callback);
    if (callback.hasHit()) {
        shot.hit = true;
        shot.point = toGlm(callback.m_hitPointWorld);
        shot.normal = glm::normalize(toGlm(callback.m_hitNormalWorld));
        // Bullet hands back the face normal of the triangle it hit; make sure it faces the shooter.
        if (glm::dot(shot.normal, shot.direction) > 0.f) {
            shot.normal = -shot.normal;
        }
        const btCollisionObject *object = callback.m_collisionObject;
        shot.object = static_cast<GameObject *>(object->getUserPointer());
        if (btRigidBody *hitBody = btRigidBody::upcast(const_cast<btCollisionObject *>(object));
            hitBody && !hitBody->isStaticOrKinematicObject()) {
            shot.dynamic = true;
            hitBody->activate(true);
            hitBody->applyImpulse(toBullet(shot.direction * SHOT_IMPULSE),
                                  callback.m_hitPointWorld - hitBody->getCenterOfMassPosition());
        }
    }
    events.shot = true;
    return true;
}

glm::vec3 Player::getEyePosition() const {
    return state.position + glm::vec3(0.f, EYE_HEIGHT, 0.f);
}

ShotResult Player::probe() const {
    ShotResult result;
    result.origin = getEyePosition();
    result.direction = getForward();
    const btVector3 from = toBullet(result.origin), to = toBullet(result.origin + result.direction * SHOT_RANGE);
    RayCallback callback(from, to, body->rb.get());
    world->rayTest(from, to, callback);
    if (callback.hasHit()) {
        result.hit = true;
        result.point = toGlm(callback.m_hitPointWorld);
        result.normal = glm::normalize(toGlm(callback.m_hitNormalWorld));
        result.object = static_cast<GameObject *>(callback.m_collisionObject->getUserPointer());
    }
    return result;
}

glm::vec3 Player::getForward() const {
    // Matches Engine::Camera3D: view = Rx(pitch) * Ry(yaw), looking down -Z.
    const float cosPitch = std::cos(state.pitch);
    return {std::sin(state.yaw) * cosPitch, -std::sin(state.pitch), -std::cos(state.yaw) * cosPitch};
}

glm::vec3 Player::getRight() const {
    return {std::cos(state.yaw), 0.f, std::sin(state.yaw)};
}

float Player::getHorizontalSpeed() const {
    return std::sqrt(state.velocity.x * state.velocity.x + state.velocity.z * state.velocity.z);
}
