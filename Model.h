#ifndef OPENGL_MODEL_H
#define OPENGL_MODEL_H

#include "MyMesh.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <plog/Log.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "bullet/btBulletDynamicsCommon.h"

class Model {
	MyMesh* myMesh;
	glm::mat4 mvp;
public:
	Model(const char* filename);
	Model(std::vector<int>* indices, std::vector<float>* vertices, std::vector<float>* output);
	~Model();

	static void loadMesh(const char* filename, std::vector<int>* indices, std::vector<float>* vertices, std::vector<float>* output);

	void draw();

	glm::mat4 getMVP();
};


#endif //OPENGL_MODEL_H
