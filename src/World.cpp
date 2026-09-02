#include "World.h"

World::World() {
    collisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();

    dispatcher = std::make_unique<btCollisionDispatcher>(collisionConfiguration.get());
    overlappingPairCache = std::make_unique<btDbvtBroadphase>();

    solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(dispatcher.get(), overlappingPairCache.get(),
                                                              solver.get(), collisionConfiguration.get());
    dynamicsWorld->setGravity(btVector3(0.f, -9.8f, 0.f));
}

void World::update(float delta) const {
    // Allow several fixed 1/60 s sub-steps per frame; with the default (1) the simulation
    // silently slows down whenever the frame takes longer than 1/60 s.
    dynamicsWorld->stepSimulation(delta, 10);
}
