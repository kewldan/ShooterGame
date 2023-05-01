#pragma once

#include <reactphysics3d/reactphysics3d.h>
#include "MyMesh.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <plog/Log.h>
#include <Shader.h>

using namespace reactphysics3d;

struct MeshData {
	std::vector<float>* vertices, * output, * normals;
	std::vector<int>* indices;
	char* texturePath;

	MeshData();
	~MeshData();
};

class Model {
	float* mvp{};
public:
	MyMesh* meshes{};
	int nbMeshes{};
	RigidBody* rb{};

	Model(const char* filename, PhysicsWorld* world, PhysicsCommon* common, bool createConcaveCollider = false);
	Model(MeshData* data, int nb, PhysicsWorld* world, PhysicsCommon* common, bool createConcaveCollider = false);

	static MeshData* loadMesh(const char* filename, int* len);

    void draw(Engine::Shader *shader);

	float* getMVP();
};
