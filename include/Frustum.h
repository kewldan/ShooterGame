#pragma once

#include <glm/glm.hpp>

// Axis-aligned bounding box. Default-constructed it is "empty" (min > max) so that extend() works
// from scratch.
struct AABB {
    glm::vec3 min{1e30f}, max{-1e30f};

    void extend(const glm::vec3 &point);

    [[nodiscard]] bool isEmpty() const;

    // Bounds of the 8 transformed corners; exact for rigid transforms, conservative otherwise.
    [[nodiscard]] AABB transformed(const glm::mat4 &matrix) const;
};

// Culling counters for one render pass (chunks and merged draw calls, see GameObject::draw).
struct CullStats {
    int drawn = 0, culled = 0, drawCalls = 0;

    void reset() { drawn = culled = drawCalls = 0; }
};

// Six planes extracted from a view-projection matrix (Gribb/Hartmann); works for perspective and
// orthographic projections alike. Normals point inside.
class Frustum {
    glm::vec4 planes[6]{};
public:
    explicit Frustum(const glm::mat4 &viewProjection);

    // Conservative: true when the box is inside or touches the frustum (never a false negative).
    [[nodiscard]] bool intersects(const AABB &box) const;
};
