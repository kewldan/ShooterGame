#pragma once

#include "Camera3D.h"
#include "Frustum.h"
#include "Mesh.h"
#include "Shader.h"
#include <memory>
#include <vector>

// A point light of the map, world space. `intensity` is the linear radiance at 1 unit; the attenuation
// (pointlight.frag) is inverse-square with a window that reaches exactly 0 at `radius`.
struct PointLight {
    glm::vec3 position;
    glm::vec3 color;
    float radius;
    float intensity;
};

// The static point lights, drawn as light volumes onto the HDR image after the fullscreen sun/ambient
// pass: a low-poly sphere of the light's radius per light, additively blended, whose fragment shader
// shades the G-buffer pixel behind it. Front faces are culled and the depth test is GL_GEQUAL (no depth
// writes), so only pixels whose geometry lies in front of the sphere's back side, i.e. inside the volume,
// are shaded, and the volume keeps working with the camera inside it. Each light also gets a small
// emissive sphere at its position so it can be seen (drawn separately, after the god rays, since it is
// not in the G-buffer and would count as sky there), and the volumes are frustum culled by their
// bounding boxes.
class PointLights {
    std::unique_ptr<Engine::Shader> shader;
    std::unique_ptr<Mesh> sphere;
    unsigned int gPosition, gNormal, gAlbedo;
    // The lights that passed the culling of the last drawVolumes(), for drawBulbs().
    std::vector<const PointLight *> onScreen;

    void bindLight(const PointLight &light, const glm::mat4 &view, float sphereRadius, const glm::vec3 &color) const;

public:
    std::vector<PointLight> lights;
    // Settings.
    bool visible = true;
    float intensityScale = 1.f;  // multiplies every light's intensity
    float bulbRadius = 0.12f;    // world units
    float bulbIntensity = 5.f;   // emissive brightness of the bulb sphere (linear, > 1 so it blooms)
    // Culling counters of the last draw().
    int drawn = 0, culled = 0;

    // The G-buffer textures are read by the volume shader; they must stay alive (GBuffer re-uses the
    // same texture names on resize).
    PointLights(unsigned int gPosition, unsigned int gNormal, unsigned int gAlbedo);

    PointLights(const PointLights &) = delete;

    PointLights &operator=(const PointLights &) = delete;

    // Draws the volumes into the bound HDR framebuffer of size `width` x `height`; the G-buffer must
    // belong to the same `camera`.
    void drawVolumes(Engine::Camera3D *camera, const Frustum &frustum, int width, int height);

    // Draws the bulbs of the lights drawVolumes() found on screen (ordinary depth test, opaque).
    void drawBulbs(Engine::Camera3D *camera);

    // The lights of dust.obj: warm lamps by the doors and arches around the spawn area.
    static std::vector<PointLight> mapLights();
};
