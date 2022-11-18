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
#include <assimp/cimport.h>

using namespace reactphysics3d;

struct MeshData {
	std::vector<float>* vertices, * output, *normals;
	std::vector<int>* indices;

	MeshData();
	~MeshData();
};

class Model {
	MyMesh* myMesh;
	float* mvp;
public:
	RigidBody* rb;

	Model(const char* filename, PhysicsWorld* world, PhysicsCommon* common, bool createConcaveCollider = false);
	Model(MeshData* data, PhysicsWorld* world, PhysicsCommon* common, bool createConcaveCollider = false);
	~Model();

	static void loadMesh(const char* filename, MeshData* out);

	void draw();

	float* getMVP();
};


#endif //OPENGL_MODEL_H
