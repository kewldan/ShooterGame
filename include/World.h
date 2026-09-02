#pragma once

#include "GameObject.h"
#include <memory>

class World {
private:
    // Declaration order matters: the dynamics world references everything above it, so it is destroyed first.
    std::unique_ptr<btDefaultCollisionConfiguration> collisionConfiguration;

    std::unique_ptr<btCollisionDispatcher> dispatcher;

    std::unique_ptr<btBroadphaseInterface> overlappingPairCache;

    std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
public:
    std::unique_ptr<btDiscreteDynamicsWorld> dynamicsWorld;

    World();

    World(const World &) = delete;

    World &operator=(const World &) = delete;

    void update(float delta) const;
};
