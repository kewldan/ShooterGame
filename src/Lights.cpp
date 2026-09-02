#include "Lights.h"

#include <cmath>
#include <glm/gtc/constants.hpp>

namespace {
    // A unit UV sphere: coarse, the volume only has to enclose the light's radius, not look round.
    constexpr int SPHERE_STACKS = 8, SPHERE_SLICES = 12;

    std::unique_ptr<Mesh> makeSphere() {
        const unsigned int vertexCount = (SPHERE_STACKS + 1) * (SPHERE_SLICES + 1);
        const int indexCount = SPHERE_STACKS * SPHERE_SLICES * 6;
        auto mesh = std::make_unique<Mesh>(vertexCount, 3, indexCount);
        size_t v = 0;
        for (int stack = 0; stack <= SPHERE_STACKS; stack++) {
            const float phi = glm::pi<float>() * static_cast<float>(stack) / SPHERE_STACKS;
            for (int slice = 0; slice <= SPHERE_SLICES; slice++) {
                const float theta = glm::two_pi<float>() * static_cast<float>(slice) / SPHERE_SLICES;
                mesh->data[v++] = std::sin(phi) * std::cos(theta);
                mesh->data[v++] = std::cos(phi);
                mesh->data[v++] = std::sin(phi) * std::sin(theta);
            }
        }
        size_t i = 0;
        for (int stack = 0; stack < SPHERE_STACKS; stack++) {
            for (int slice = 0; slice < SPHERE_SLICES; slice++) {
                const unsigned int a = stack * (SPHERE_SLICES + 1) + slice, b = a + SPHERE_SLICES + 1;
                // Counter-clockwise seen from outside, so GL_BACK/GL_FRONT culling mean what they say.
                mesh->indices[i++] = a;
                mesh->indices[i++] = a + 1;
                mesh->indices[i++] = b;
                mesh->indices[i++] = a + 1;
                mesh->indices[i++] = b + 1;
                mesh->indices[i++] = b;
            }
        }
        mesh->upload();
        mesh->addParameter(0, 3);
        return mesh;
    }
}

PointLights::PointLights(unsigned int gPosition, unsigned int gNormal, unsigned int gAlbedo)
        : gPosition(gPosition), gNormal(gNormal), gAlbedo(gAlbedo) {
    shader = std::make_unique<Engine::Shader>("pointlight");
    shader->bind();
    shader->upload("gPosition", 0);
    shader->upload("gNormal", 1);
    shader->upload("gAlbedoSpec", 2);
    sphere = makeSphere();
}

void PointLights::bindLight(const PointLight &light, const glm::mat4 &view, float sphereRadius,
                            const glm::vec3 &color) const {
    shader->upload("position", glm::vec3(view * glm::vec4(light.position, 1.f)));
    shader->upload("color", color);
    shader->upload("radius", sphereRadius);
}

void PointLights::drawVolumes(Engine::Camera3D *camera, const Frustum &frustum, int width, int height) {
    drawn = culled = 0;
    onScreen.clear();
    if (!visible || lights.empty()) {
        return;
    }
    const glm::mat4 &view = camera->getView();
    shader->bind();
    shader->upload("proj", camera->getProjection());
    shader->upload("screenSize", glm::vec2(width, height));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);

    for (const PointLight &light: lights) {
        AABB box;
        box.extend(light.position - light.radius);
        box.extend(light.position + light.radius);
        if (frustum.intersects(box)) {
            onScreen.push_back(&light);
        } else {
            culled++;
        }
    }
    drawn = static_cast<int>(onScreen.size());

    // Volumes: additive light, back faces only, where the scene lies in front of them.
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_GEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    shader->upload("emissive", 0);
    for (const PointLight *light: onScreen) {
        bindLight(*light, view, light->radius, light->color * light->intensity * intensityScale);
        sphere->draw();
    }
    glCullFace(GL_BACK);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

void PointLights::drawBulbs(Engine::Camera3D *camera) {
    if (onScreen.empty()) {
        return;
    }
    // Small opaque emissive spheres, ordinary depth test so walls hide them.
    const glm::mat4 &view = camera->getView();
    shader->bind();
    shader->upload("proj", camera->getProjection());
    shader->upload("emissive", 1);
    for (const PointLight *light: onScreen) {
        bindLight(*light, view, bulbRadius, light->color * bulbIntensity);
        sphere->draw();
    }
    glBindVertexArray(0);
}

std::vector<PointLight> PointLights::mapLights() {
    // Warm tungsten-ish lamps about 4-5 units (2-2.5 m) above the floor. The spawn corridor runs
    // north-south (x = -3..9, floor y = -11.1 rising to -6.5 towards +z), the big door closes its north
    // end (z = -21, lintel at -4.4), the decorated arch in its west wall (z = -9..-14, top at -6.3) opens
    // into a large room with a ceiling at -4.2, and a raised ledge runs along its east wall.
    // Radii in world units (about 0.5 m each).
    const glm::vec3 warm(1.f, 0.72f, 0.42f), amber(1.f, 0.6f, 0.3f);
    return {
            {{4.5f, -7.5f, -20.f}, warm, 12.f, 30.f},    // over the big door
            {{-3.5f, -7.8f, -11.5f}, amber, 10.f, 25.f}, // in the arch
            {{7.5f, -7.5f, -6.f}, warm, 10.f, 25.f},     // by the crates, at the east ledge
            {{-12.f, -6.5f, -10.f}, amber, 12.f, 30.f},  // the western room, behind the arch
            {{-22.f, -6.5f, -12.f}, warm, 12.f, 25.f},   // the western room, by the pillar
            {{4.f, -7.5f, -27.f}, warm, 12.f, 30.f},     // beyond the big door
            {{3.f, -4.5f, 9.f}, warm, 12.f, 25.f},       // the corridor behind the spawn
            {{3.f, -2.f, 19.f}, amber, 12.f, 25.f},      // its far (higher) end
    };
}
