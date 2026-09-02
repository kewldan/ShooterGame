#pragma once

#include "Frustum.h"
#include "Mesh.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <memory>
#include <plog/Log.h>
#include <Shader.h>
#include <vector>
#include <btBulletDynamicsCommon.h>

class GameObject {
public:
    // The unit of frustum culling: an index range of one mesh with its own local-space bounds. A static
    // map is split into one chunk per (material, XZ grid cell), everything else has one chunk per mesh.
    struct Chunk {
        size_t mesh;
        int firstIndex, indexCount;
        AABB bounds;
    };

private:
    float mvp[16]{};
    bool castShadows = true;
    btDynamicsWorld *world;
    // World-space bounds of every chunk for the model matrix in `mvp`; refreshed by draw() when the
    // body moved (a static map pays for the transform once).
    std::vector<AABB> worldBounds;
    glm::mat4 worldBoundsModel{0.f};
    // The triangles of the model when it doubles as its own (static) collision shape; the shape keeps
    // a pointer into it, so it lives as long as the object.
    std::unique_ptr<btTriangleMesh> triangleMesh;

    void updateWorldBounds();

public:
    // One mesh per OBJ material.
    std::vector<Mesh> meshes;
    // Sorted by mesh, and within a mesh by grid cell, so that neighbouring visible chunks are
    // contiguous index ranges and can be drawn with a single call.
    std::vector<Chunk> chunks;
    btVector3 localInertia{0.f, 0.f, 0.f};
    // Declaration order matters: the body references the shape and motion state, so it is destroyed first.
    std::unique_ptr<btCollisionShape> collisionShape;
    std::unique_ptr<btDefaultMotionState> motionState;
    std::unique_ptr<btRigidBody> rb;

    // Takes ownership of `shape`. `world` must outlive the object. A `chunkSize` > 0 splits the meshes into
    // an XZ grid of that cell size for frustum culling (meant for the static map, see loadMeshes()).
    // With a null `shape` (and `mass` 0) the model's own triangles become the collision shape, so that
    // rays and props meet the real walls and floors of a map.
    explicit GameObject(btDynamicsWorld *world, const char *path = nullptr, float mass = 0.f,
                        btCollisionShape *shape = nullptr, btVector3 position = btVector3(0.f, 0.f, 0.f),
                        float chunkSize = 0.f);

    ~GameObject();

    GameObject(const GameObject &) = delete;

    GameObject &operator=(const GameObject &) = delete;

    // Loads `data/meshes/<path>` into `meshes`/`chunks`, one Mesh per OBJ material. With `chunkSize` > 0
    // the triangles of every material are bucketed (by centroid) into chunkSize x chunkSize cells in local
    // XZ and reordered so that each non-empty cell is one Chunk (index range + AABB), so off-screen parts
    // of a big map are not drawn. Without it every mesh is a single chunk bounded by its vertices. When
    // `triangles` is given every triangle is appended to it too (collision mesh). Usable without a
    // GameObject (the view-model weapon). Returns false when the file could not be loaded.
    static bool loadObj(const char *path, float chunkSize, std::vector<Mesh> &meshes, std::vector<Chunk> &chunks,
                        btTriangleMesh *triangles);

    // loadObj() into this object (and into its collision triangles when it has them).
    void loadMeshes(const char *path, float chunkSize = 0.f);

    // Replaces the meshes with an axis-aligned box of the given half extents, textured with
    // `data/textures/<texture>` when given (matches a btBoxShape of the same extents).
    void setBoxMesh(const glm::vec3 &halfExtents, const char *texture = nullptr);

    // Draws every chunk whose world-space AABB intersects `frustum` (all of them when it is null), merging
    // runs of visible chunks into one draw call, and counts them into `stats` when given. `shader` must be bound.
    void draw(Engine::Shader *shader, const Frustum *frustum = nullptr, CullStats *stats = nullptr);

    void setCastShadows(bool value);

    [[nodiscard]] bool isCastShadows() const;
};
