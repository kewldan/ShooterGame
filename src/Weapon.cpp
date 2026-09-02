#include "Weapon.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <glm/gtc/constants.hpp>

namespace {
    // The view-model sits well within a metre of the camera.
    constexpr float NEAR = 0.01f, FAR = 5.f;

    glm::mat4 viewModelProjection(float hFov, float aspect) {
        const float vFov = 2.f * std::atan(std::tan(glm::radians(hFov) / 2.f) / aspect);
        return glm::perspective(vFov, aspect, NEAR, FAR);
    }

    // Exponential approach: the fraction of the remaining distance covered in `delta` seconds.
    float approach(float rate, float delta) {
        return 1.f - std::exp(-rate * delta);
    }
}

unsigned int Weapon::createFlashTexture() {
    constexpr int SIZE = 64;
    std::vector<unsigned char> pixels(SIZE * SIZE * 3);
    for (int y = 0; y < SIZE; y++) {
        for (int x = 0; x < SIZE; x++) {
            const float dx = (x + 0.5f) / SIZE * 2.f - 1.f, dy = (y + 0.5f) / SIZE * 2.f - 1.f;
            const float r2 = dx * dx + dy * dy;
            const float angle = std::atan2(dy, dx);
            const float spikes = std::pow(std::abs(std::cos(2.f * angle)), 12.f) * std::exp(-r2 * 2.5f) * 0.7f;
            const float value = std::clamp(std::exp(-r2 * 9.f) + spikes, 0.f, 1.f);
            unsigned char *p = &pixels[(y * SIZE + x) * 3];
            p[0] = static_cast<unsigned char>(value * 255.f);
            p[1] = static_cast<unsigned char>(value * value * 0.8f * 255.f);
            p[2] = static_cast<unsigned char>(value * value * value * 0.45f * 255.f);
        }
    }
    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SIZE, SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}

Weapon::Weapon(const char *meshPath) : flashQuad(4, 8, 6) {
    GameObject::loadObj(meshPath, 0.f, meshes, chunks, nullptr);
    shader = std::make_unique<Engine::Shader>("viewmodel");
    shader->bind();
    shader->upload("aTexture", 0);

    // A unit quad in the XY plane facing +Z (position, texCoord, normal like the OBJ meshes).
    flashQuad.data = {
            -0.5f, -0.5f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f,
            0.5f, -0.5f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f,
            0.5f, 0.5f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f,
            -0.5f, 0.5f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f,
    };
    flashQuad.indices = {0, 1, 2, 0, 2, 3};
    flashQuad.upload();
    flashQuad.addParameter(0, 3);
    flashQuad.addParameter(1, 2);
    flashQuad.addParameter(2, 3);
    flashTexture = createFlashTexture();
}

Weapon::~Weapon() {
    glDeleteTextures(1, &flashTexture);
}

void Weapon::kick() {
    recoil = std::min(recoil + 1.f, 1.5f);
    flashTimer = flashDuration;
    flashSpin = static_cast<float>(std::rand() % 360);
}

void Weapon::update(glm::vec2 mouseDelta, float speed, bool grounded, bool aiming, float delta) {
    delta = std::clamp(delta, 0.f, 0.1f);

    // Aiming down the sights: blend towards the centred offset, damp the other motions.
    aim += ((aiming ? 1.f : 0.f) - aim) * approach(10.f, delta);
    const float damping = 1.f - 0.8f * aim;

    // Sway: the weapon lags behind a turn, proportional to the mouse speed (pixels per second).
    glm::vec2 target(0.f);
    if (delta > 0.f) {
        target = glm::clamp(mouseDelta / delta * swayAmount * damping, -swayMax, swayMax);
    }
    sway += (target - sway) * approach(swaySpeed, delta);

    // Bob: a figure of eight while walking; its amplitude fades in and out with the speed.
    const float bobTarget = grounded ? std::clamp(speed / 30.f, 0.f, 1.f) : 0.f;
    bobBlend += (bobTarget - bobBlend) * approach(8.f, delta);
    bobPhase += delta * glm::two_pi<float>() * bobFrequency * bobBlend;
    if (bobPhase > glm::two_pi<float>()) bobPhase -= glm::two_pi<float>();
    const float amplitude = bobAmount * bobBlend * damping;
    bob = glm::vec3(std::sin(bobPhase) * amplitude, std::sin(2.f * bobPhase) * amplitude * 0.5f, 0.f);

    // Recoil: an instant kick (see kick()) that recovers exponentially.
    recoil -= recoil * approach(recoilRecovery, delta);
    flashTimer = std::max(flashTimer - delta, 0.f);

    glm::vec3 offset = glm::mix(hipOffset, aimOffset, aim) + bob;
    offset.x -= sway.x * 0.3f;
    offset.y += sway.y * 0.3f;
    offset.z += recoil * recoilKick;
    model = glm::translate(glm::mat4(1.f), offset);
    model = glm::rotate(model, glm::radians(recoil * recoilAngle), glm::vec3(1.f, 0.f, 0.f));
    // Turning right (sway.x > 0) leaves the muzzle pointing a little to the left, and the same for pitch.
    model = glm::rotate(model, sway.x, glm::vec3(0.f, 1.f, 0.f));
    model = glm::rotate(model, sway.y, glm::vec3(1.f, 0.f, 0.f));
    model = glm::rotate(model, glm::radians(baseRotation.y), glm::vec3(0.f, 1.f, 0.f));
    model = glm::rotate(model, glm::radians(baseRotation.x), glm::vec3(1.f, 0.f, 0.f));
    model = glm::rotate(model, glm::radians(baseRotation.z), glm::vec3(0.f, 0.f, 1.f));
    model = glm::scale(model, glm::vec3(scale));
}

glm::vec3 Weapon::getMuzzleWorld(const glm::mat4 &proj, const glm::mat4 &inverseView, float aspect) const {
    const glm::vec3 muzzleView = glm::vec3(model * glm::vec4(muzzleLocal, 1.f));
    // Where the muzzle lands on screen under the view-model projection...
    const glm::vec4 clip = viewModelProjection(fov, aspect) * glm::vec4(muzzleView, 1.f);
    const glm::vec2 ndc = glm::vec2(clip) / clip.w;
    // ...and the view-space point at the same depth that the main projection puts there.
    const float depth = -muzzleView.z;
    const glm::vec3 remapped(ndc.x * depth / proj[0][0], ndc.y * depth / proj[1][1], muzzleView.z);
    return glm::vec3(inverseView * glm::vec4(remapped, 1.f));
}

void Weapon::draw(float aspect, const glm::vec3 &sunDir, const glm::vec3 &sunColor, const glm::vec3 &ambientColor) {
    shader->bind();
    shader->upload("proj", viewModelProjection(fov, aspect));
    shader->upload("model", model);
    shader->upload("sunDir", sunDir);
    shader->upload("sunColor", sunColor);
    shader->upload("ambientColor", ambientColor);
    shader->upload("unlit", 0);
    glActiveTexture(GL_TEXTURE0);
    for (const Mesh &mesh: meshes) {
        if (mesh.hasTexture()) {
            mesh.texture->bind();
        }
        shader->upload("hasTexture", mesh.hasTexture() ? 1 : 0);
        mesh.draw();
    }

    if (flashTimer > 0.f) {
        // A camera-facing, additive sprite just in front of the muzzle; drawn over the barrel on purpose.
        const glm::vec3 muzzle = glm::vec3(model * glm::vec4(muzzleLocal, 1.f));
        glm::mat4 flash = glm::translate(glm::mat4(1.f), muzzle);
        flash = glm::rotate(flash, glm::radians(flashSpin), glm::vec3(0.f, 0.f, 1.f));
        flash = glm::scale(flash, glm::vec3(flashSize * (1.f + aim * 0.3f)));
        shader->upload("model", flash);
        shader->upload("unlit", 1);
        shader->upload("emissive", flashIntensity);
        shader->upload("hasTexture", 1);
        glBindTexture(GL_TEXTURE_2D, flashTexture);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        flashQuad.draw();
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }
    glBindVertexArray(0);
}

float Weapon::getAim() const {
    return aim;
}
