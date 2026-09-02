#include "Sun.h"

#include <algorithm>
#include <cmath>

namespace {
    // Where the sprite sits along the sun direction: well inside the camera's far plane (300).
    constexpr float SPRITE_DISTANCE = 200.f;

    void setupTexture(unsigned int texture, int w, int h) {
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, w, h, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Samples taken past the edge (the sun near or beyond it) find nothing, not a smeared border.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        const float black[] = {0.f, 0.f, 0.f, 0.f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, black);
    }

    float smoothstep(float edge0, float edge1, float x) {
        const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
        return t * t * (3.f - 2.f * t);
    }
}

Sun::Sun(const glm::vec3 &direction, const glm::vec3 &color, int width, int height)
        : direction(glm::normalize(direction)), color(color), w(width), h(height) {
    // The sprite: a unit quad, the vertex shader spans it in view space.
    static const float corners[] = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f};
    glGenVertexArrays(1, &spriteVAO);
    glGenBuffers(1, &spriteVBO);
    glBindVertexArray(spriteVAO);
    glBindBuffer(GL_ARRAY_BUFFER, spriteVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindVertexArray(0);

    glGenTextures(1, &maskTexture);
    glGenTextures(1, &raysTexture);
    allocate();
    glGenFramebuffers(1, &maskFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, maskFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, maskTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        PLOGE << "God rays mask framebuffer not complete!";
    }
    glGenFramebuffers(1, &raysFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, raysFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, raysTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        PLOGE << "God rays framebuffer not complete!";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    spriteShader = std::make_unique<Engine::Shader>("sun");
    maskShader = std::make_unique<Engine::Shader>("godraysMask");
    raysShader = std::make_unique<Engine::Shader>("godrays");
    // The tent upsample of the bloom chain does exactly what adding the half-resolution rays needs.
    addShader = std::make_unique<Engine::Shader>("bloomUp");
    maskShader->bind();
    maskShader->upload("hdr", 0);
    maskShader->upload("gNormal", 1);
    raysShader->bind();
    raysShader->upload("mask", 0);
    addShader->bind();
    addShader->upload("source", 0);
}

Sun::~Sun() {
    glDeleteFramebuffers(1, &raysFBO);
    glDeleteFramebuffers(1, &maskFBO);
    glDeleteTextures(1, &raysTexture);
    glDeleteTextures(1, &maskTexture);
    glDeleteBuffers(1, &spriteVBO);
    glDeleteVertexArrays(1, &spriteVAO);
}

void Sun::allocate() {
    setupTexture(maskTexture, std::max(w / 2, 1), std::max(h / 2, 1));
    setupTexture(raysTexture, std::max(w / 2, 1), std::max(h / 2, 1));
}

void Sun::resize(int width, int height) {
    w = std::max(width, 1);
    h = std::max(height, 1);
    allocate();
}

void Sun::draw(Engine::Camera3D *camera, unsigned int hdrFBO, unsigned int hdrTexture, unsigned int gNormal) {
    const glm::mat4 &proj = camera->getProjection();
    const glm::vec3 viewDir = glm::normalize(glm::mat3(camera->getView()) * direction);
    fade = 0.f;
    // Behind the camera (the camera looks down -Z in view space): nothing to draw.
    if (viewDir.z >= -1e-3f) {
        return;
    }
    const glm::vec3 centre = viewDir * SPRITE_DISTANCE;
    const glm::vec4 clip = proj * glm::vec4(centre, 1.f);
    const glm::vec2 ndc = glm::vec2(clip) / clip.w;
    screenPosition = ndc * 0.5f + 0.5f;
    // The rays fade as the disk leaves the screen; with it far outside nothing is left to blur.
    fade = 1.f - smoothstep(1.f, 1.6f, std::max(std::abs(ndc.x), std::abs(ndc.y)));

    // 1. The sprite: additive, depth tested against the scene, never writing depth.
    const glm::vec3 helper = std::abs(viewDir.y) < 0.99f ? glm::vec3(0.f, 1.f, 0.f) : glm::vec3(1.f, 0.f, 0.f);
    const glm::vec3 right = glm::normalize(glm::cross(viewDir, helper));
    const glm::vec3 up = glm::cross(right, viewDir);
    // Twice the disk so the glow has room.
    const float size = SPRITE_DISTANCE * std::tan(glm::radians(angularRadius)) * 2.f;
    spriteShader->bind();
    spriteShader->upload("proj", proj);
    spriteShader->upload("centre", centre);
    spriteShader->upload("right", right * size);
    spriteShader->upload("up", up * size);
    spriteShader->upload("color", color);
    spriteShader->upload("diskIntensity", diskIntensity);
    spriteShader->upload("glowIntensity", glowIntensity);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glBindVertexArray(spriteVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    if (!godRays || fade <= 0.f) {
        return;
    }

    // 2. Occlusion mask at half resolution: what shines (sky, sun) and what blocks it (geometry).
    const int hw = std::max(w / 2, 1), hh = std::max(h / 2, 1);
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, hw, hh);
    glBindFramebuffer(GL_FRAMEBUFFER, maskFBO);
    maskShader->bind();
    maskShader->upload("threshold", raysThreshold);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    quad.draw();

    // 3. Radial blur of the mask towards the sun.
    glBindFramebuffer(GL_FRAMEBUFFER, raysFBO);
    raysShader->bind();
    raysShader->upload("sunPosition", screenPosition);
    raysShader->upload("density", raysDensity);
    raysShader->upload("decay", raysDecay);
    raysShader->upload("weight", raysWeight);
    raysShader->upload("exposure", raysStrength * fade);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, maskTexture);
    quad.draw();

    // 4. Add the rays onto the HDR image.
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glViewport(0, 0, w, h);
    addShader->bind();
    addShader->upload("sourceTexel", glm::vec2(1.f / static_cast<float>(hw), 1.f / static_cast<float>(hh)));
    glBindTexture(GL_TEXTURE_2D, raysTexture);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    quad.draw();
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

float Sun::getFade() const {
    return fade;
}
