#pragma once

#include "GameObject.h"

class World {
private:
    btDefaultCollisionConfiguration *collisionConfiguration;

    btCollisionDispatcher *dispatcher;

    btBroadphaseInterface *overlappingPairCache;

    btSequentialImpulseConstraintSolver *solver;
public:
    btDiscreteDynamicsWorld *dynamicsWorld;

    World();

    void update(float delta) const;
};
