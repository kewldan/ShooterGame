#ifndef OPENGL_MODEL_H
#define OPENGL_MODEL_H

#include <reactphysics3d/reactphysics3d.h>
#include "MyMesh.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <plog/Log.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace reactphysics3d;

class Model {
	MyMesh* myMesh;
	glm::mat4 mvp;
public:
	RigidBody* rb;
	PhysicsWorld* world;
	std::vector<int>* indices;
	std::vector<float>* vertices;

	Model(std::string filename, PhysicsWorld* world, PhysicsCommon* common, bool createConcaveCollider = false);
	~Model();

	void draw();

	void print();

	glm::mat4 getMVP();
};


#endif //OPENGL_MODEL_H
