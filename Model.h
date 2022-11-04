#ifndef OPENGL_MODEL_H
#define OPENGL_MODEL_H

#include <reactphysics3d/reactphysics3d.h>
#include "MyMesh.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <plog/Log.h>

using namespace reactphysics3d;

class Model {
    MyMesh *mesh;
    glm::mat4 mvp;
public:
    RigidBody *rb;
    std::vector<int> *indices;
    std::vector<float> *vertices;

    Model(std::string filename, PhysicsWorld *world, PhysicsCommon *common, bool createConcaveCollider = false);

    glm::mat4 getMVP();

    MyMesh *getMesh() const;
};


#endif //OPENGL_MODEL_H
