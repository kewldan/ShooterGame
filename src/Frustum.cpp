#include "Frustum.h"

void AABB::extend(const glm::vec3 &point) {
    min = glm::min(min, point);
    max = glm::max(max, point);
}

bool AABB::isEmpty() const {
    return min.x > max.x || min.y > max.y || min.z > max.z;
}

AABB AABB::transformed(const glm::mat4 &matrix) const {
    AABB result;
    for (int i = 0; i < 8; i++) {
        const glm::vec3 corner((i & 1) ? max.x : min.x, (i & 2) ? max.y : min.y, (i & 4) ? max.z : min.z);
        result.extend(glm::vec3(matrix * glm::vec4(corner, 1.f)));
    }
    return result;
}

Frustum::Frustum(const glm::mat4 &m) {
    // glm is column-major: row(i) = (m[0][i], m[1][i], m[2][i], m[3][i]).
    const auto row = [&m](int i) { return glm::vec4(m[0][i], m[1][i], m[2][i], m[3][i]); };
    const glm::vec4 r0 = row(0), r1 = row(1), r2 = row(2), r3 = row(3);
    planes[0] = r3 + r0; // left
    planes[1] = r3 - r0; // right
    planes[2] = r3 + r1; // bottom
    planes[3] = r3 - r1; // top
    planes[4] = r3 + r2; // near
    planes[5] = r3 - r2; // far
    for (auto &plane: planes) {
        const float length = glm::length(glm::vec3(plane));
        if (length > 0.f) plane /= length;
    }
}

bool Frustum::intersects(const AABB &box) const {
    for (const auto &plane: planes) {
        // The corner furthest along the plane normal: if even that is behind the plane, the box is out.
        const glm::vec3 corner(plane.x >= 0.f ? box.max.x : box.min.x,
                               plane.y >= 0.f ? box.max.y : box.min.y,
                               plane.z >= 0.f ? box.max.z : box.min.z);
        if (glm::dot(glm::vec3(plane), corner) + plane.w < 0.f) {
            return false;
        }
    }
    return true;
}
