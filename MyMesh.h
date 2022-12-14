#ifndef OPENGL_MESH_H
#define OPENGL_MESH_H

#include "glad/glad.h"
#include <vector>
#include "plog/Log.h"
#include "Texture.h"

class MyMesh {
	unsigned int VAO, EBO, VBO;
	unsigned long long vertexOffset;
	
public:
	Texture* texture;
	int indicesCount, stride;
	MyMesh(std::vector<float>* vertices, std::vector<int>* indices, unsigned int vertexSize, char* label = nullptr);

	void draw() const;

	void addParameter(int location, int size);

	void addParameter(int location, int size, bool normalized);

	bool hasTexture();
};


#endif //OPENGL_MESH_H
