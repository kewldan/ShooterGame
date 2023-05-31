#pragma once

#include "bullet/btBulletDynamicsCommon.h"

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
