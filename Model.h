#ifndef OPENGL_MODEL_H
#define OPENGL_MODEL_H

#include "MyMesh.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <plog/Log.h>
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

class Model {
	std::vector<MyMesh> meshes;
	glm::mat4 mvp;
	void processNode(aiNode* node, const aiScene* scene);
	MyMesh processMesh(aiMesh* mesh, const aiScene* scene);
public:
	std::vector<int>* indices;
	std::vector<float>* vertices;

	Model(std::string filename);
	~Model();

	glm::mat4 getMVP();

	void draw();
};


#endif //OPENGL_MODEL_H
