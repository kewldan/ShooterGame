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

	Model(const char* filename, PhysicsWorld* world, PhysicsCommon* common, bool createConcaveCollider = false);
	Model(std::vector<int>* indices, std::vector<float>* vertices, std::vector<float>* output, PhysicsWorld* world, PhysicsCommon* common, bool createConcaveCollider = false);
	~Model();

	static void loadMesh(const char* filename, std::vector<int>* indices, std::vector<float>* vertices, std::vector<float>* output);

	void draw();

	glm::mat4 getMVP();
};


#endif //OPENGL_MODEL_H
