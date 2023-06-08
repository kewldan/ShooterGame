#pragma once

#include "Mesh.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <plog/Log.h>
#include <Shader.h>
#include "bullet/btBulletDynamicsCommon.h"

class GameObject {
private:
    float* mvp;
    bool castShadows;
public:
	Mesh* meshes;
    size_t nbMeshes;
    btVector3 localInertia;
    btCollisionShape* collisionShape;
    btDefaultMotionState* motionState;
    btRigidBody* rb;

    explicit GameObject(btDynamicsWorld* world, const char* path = nullptr, float mass = 0.f, btCollisionShape* shape = nullptr, btVector3 position = btVector3(0.f, 0.f, 0.f));

    void loadMeshes(const char* path);
    void draw(Engine::Shader *shader);
    void setCastShadows(bool value);
    bool isCastShadows();
};
