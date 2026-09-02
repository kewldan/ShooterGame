#include "Effects.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <glm/gtc/constants.hpp>

namespace {
    constexpr int DECAL_VERTICES = 4, DECAL_INDICES = 6;
    constexpr int TRACER_VERTICES = 6; // two triangles, not indexed

    float smoothstep(float edge0, float edge1, float x) {
        const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
        return t * t * (3.f - 2.f * t);
    }
}

unsigned int Effects::createBulletHoleTexture() {
    constexpr int SIZE = 64;
    std::vector<unsigned char> pixels(SIZE * SIZE * 4);
    std::mt19937 noise(7);
    std::uniform_real_distribution<float> grain(-1.f, 1.f);
    for (int y = 0; y < SIZE; y++) {
        for (int x = 0; x < SIZE; x++) {
            const float dx = (x + 0.5f) / SIZE * 2.f - 1.f, dy = (y + 0.5f) / SIZE * 2.f - 1.f;
            const float r = std::sqrt(dx * dx + dy * dy);
            const float g = grain(noise);
            // Opaque core, ragged fade-out towards the rim.
            const float alpha = 1.f - smoothstep(0.5f, 1.f, r + g * 0.12f);
            // Nearly black centre, a lighter scorched ring around it.
            const float shade = 0.04f + 0.3f * smoothstep(0.2f, 0.8f, r) + g * 0.04f;
            unsigned char *p = &pixels[(y * SIZE + x) * 4];
            p[0] = static_cast<unsigned char>(std::clamp(shade, 0.f, 1.f) * 255.f);
            p[1] = static_cast<unsigned char>(std::clamp(shade * 0.95f, 0.f, 1.f) * 255.f);
            p[2] = static_cast<unsigned char>(std::clamp(shade * 0.9f, 0.f, 1.f) * 255.f);
            p[3] = static_cast<unsigned char>(std::clamp(alpha, 0.f, 1.f) * 255.f);
        }
    }
    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SIZE, SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}

Effects::Effects() {
    shader = std::make_unique<Engine::Shader>("effect");
    shader->bind();
    shader->upload("aTexture", 0);
    decalTexture = createBulletHoleTexture();

    const auto describeVertex = [] {
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<const void *>(offsetof(Vertex, position)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<const void *>(offsetof(Vertex, texCoord)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<const void *>(offsetof(Vertex, color)));
    };

    // Decals: a fixed-size vertex buffer updated slot by slot, indexed with a static quad pattern.
    glGenVertexArrays(1, &decalVAO);
    glBindVertexArray(decalVAO);
    glGenBuffers(1, &decalVBO);
    glBindBuffer(GL_ARRAY_BUFFER, decalVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_DECALS * DECAL_VERTICES * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
    describeVertex();
    std::vector<unsigned int> indices(MAX_DECALS * DECAL_INDICES);
    for (int i = 0; i < MAX_DECALS; i++) {
        const unsigned int base = i * DECAL_VERTICES;
        const unsigned int quad[DECAL_INDICES] = {base, base + 1, base + 2, base, base + 2, base + 3};
        std::copy(std::begin(quad), std::end(quad), indices.begin() + i * DECAL_INDICES);
    }
    glGenBuffers(1, &decalEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, decalEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                 indices.data(), GL_STATIC_DRAW);

    // Tracers: rebuilt every frame (they face the camera), so a plain streaming buffer.
    glGenVertexArrays(1, &tracerVAO);
    glBindVertexArray(tracerVAO);
    glGenBuffers(1, &tracerVBO);
    glBindBuffer(GL_ARRAY_BUFFER, tracerVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_TRACERS * TRACER_VERTICES * sizeof(Vertex), nullptr, GL_STREAM_DRAW);
    describeVertex();
    glBindVertexArray(0);
    tracers.reserve(MAX_TRACERS);
}

Effects::~Effects() {
    glDeleteBuffers(1, &tracerVBO);
    glDeleteVertexArrays(1, &tracerVAO);
    glDeleteBuffers(1, &decalEBO);
    glDeleteBuffers(1, &decalVBO);
    glDeleteVertexArrays(1, &decalVAO);
    glDeleteTextures(1, &decalTexture);
}

void Effects::addDecal(const glm::vec3 &point, const glm::vec3 &normal) {
    // Tangent frame on the surface, spun by a random angle so the holes do not all look alike.
    const glm::vec3 helper = std::abs(normal.y) < 0.9f ? glm::vec3(0.f, 1.f, 0.f) : glm::vec3(1.f, 0.f, 0.f);
    glm::vec3 tangent = glm::normalize(glm::cross(helper, normal));
    glm::vec3 bitangent = glm::cross(normal, tangent);
    std::uniform_real_distribution<float> spin(0.f, glm::two_pi<float>());
    const float angle = spin(rng);
    const glm::vec3 t = tangent * std::cos(angle) + bitangent * std::sin(angle);
    const glm::vec3 b = glm::cross(normal, t);

    const glm::vec3 centre = point + normal * decalOffset;
    const float half = decalSize * 0.5f;
    const glm::vec4 white(1.f);
    const Vertex quad[DECAL_VERTICES] = {
            {centre - t * half - b * half, {0.f, 0.f}, white},
            {centre + t * half - b * half, {1.f, 0.f}, white},
            {centre + t * half + b * half, {1.f, 1.f}, white},
            {centre - t * half + b * half, {0.f, 1.f}, white},
    };
    glBindBuffer(GL_ARRAY_BUFFER, decalVBO);
    glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLintptr>(decalNext) * DECAL_VERTICES * sizeof(Vertex), sizeof(quad),
                    quad);
    decalNext = (decalNext + 1) % MAX_DECALS;
    decalCount = std::min(decalCount + 1, MAX_DECALS);
}

void Effects::addTracer(const glm::vec3 &from, const glm::vec3 &to) {
    if (tracers.size() >= MAX_TRACERS) {
        tracers.erase(tracers.begin());
    }
    tracers.push_back({from, to, 0.f});
}

void Effects::update(float delta) {
    for (Tracer &tracer: tracers) {
        tracer.age += delta;
    }
    std::erase_if(tracers, [&](const Tracer &tracer) { return tracer.age >= tracerLife; });
}

void Effects::draw(const glm::mat4 &proj, const glm::mat4 &view, const glm::vec3 &cameraPosition) {
    if (decalCount == 0 && tracers.empty()) {
        return;
    }
    shader->bind();
    shader->upload("proj", proj);
    shader->upload("view", view);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    // The quads are built in world space with whatever winding the surface normal gives.
    glDisable(GL_CULL_FACE);

    if (decalCount > 0) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.f, -1.f);
        glBindTexture(GL_TEXTURE_2D, decalTexture);
        shader->upload("hasTexture", 1);
        glBindVertexArray(decalVAO);
        glDrawElements(GL_TRIANGLES, decalCount * DECAL_INDICES, GL_UNSIGNED_INT, nullptr);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    if (!tracers.empty()) {
        std::vector<Vertex> vertices;
        vertices.reserve(tracers.size() * TRACER_VERTICES);
        for (const Tracer &tracer: tracers) {
            const glm::vec3 along = tracer.to - tracer.from;
            if (glm::dot(along, along) < 1e-6f) continue;
            // A strip perpendicular to both the flight path and the line of sight: always seen face-on.
            glm::vec3 side = glm::cross(glm::normalize(along), cameraPosition - tracer.from);
            const float length = glm::length(side);
            if (length < 1e-6f) continue;
            side *= tracerWidth * 0.5f / length;
            const float fade = 1.f - tracer.age / tracerLife;
            const glm::vec4 color(tracerColor, fade);
            const Vertex a{tracer.from - side, {0.f, 0.f}, color}, b{tracer.from + side, {1.f, 0.f}, color};
            const Vertex c{tracer.to + side, {1.f, 1.f}, color}, d{tracer.to - side, {0.f, 1.f}, color};
            vertices.insert(vertices.end(), {a, b, c, a, c, d});
        }
        if (!vertices.empty()) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive: a hot streak of light
            shader->upload("hasTexture", 0);
            glBindVertexArray(tracerVAO);
            glBindBuffer(GL_ARRAY_BUFFER, tracerVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                            vertices.data());
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
        }
    }

    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

int Effects::getDecalCount() const {
    return decalCount;
}

int Effects::getTracerCount() const {
    return static_cast<int>(tracers.size());
}
