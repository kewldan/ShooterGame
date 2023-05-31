#include "World.h"

World::World() {
    collisionConfiguration = new btDefaultCollisionConfiguration;

    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    overlappingPairCache = new btDbvtBroadphase;

    solver = new btSequentialImpulseConstraintSolver;
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0.f, -9.8f, 0.f));
}

void World::update(float delta) const {
    dynamicsWorld->stepSimulation(delta);
}
