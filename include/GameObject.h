#pragma once

#include "Mesh.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <memory>
#include <plog/Log.h>
#include <Shader.h>
#include <vector>
#include <btBulletDynamicsCommon.h>

class GameObject {
private:
    float mvp[16]{};
    bool castShadows = true;
    btDynamicsWorld *world;
public:
    std::vector<Mesh> meshes;
    btVector3 localInertia{0.f, 0.f, 0.f};
    // Declaration order matters: the body references the shape and motion state, so it is destroyed first.
    std::unique_ptr<btCollisionShape> collisionShape;
    std::unique_ptr<btDefaultMotionState> motionState;
    std::unique_ptr<btRigidBody> rb;

    // Takes ownership of `shape`. `world` must outlive the object.
    explicit GameObject(btDynamicsWorld *world, const char *path = nullptr, float mass = 0.f,
                        btCollisionShape *shape = nullptr, btVector3 position = btVector3(0.f, 0.f, 0.f));

    ~GameObject();

    GameObject(const GameObject &) = delete;

    GameObject &operator=(const GameObject &) = delete;

    void loadMeshes(const char *path);

    void draw(Engine::Shader *shader);

    void setCastShadows(bool value);

    [[nodiscard]] bool isCastShadows() const;
};
