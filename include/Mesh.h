#pragma once

#include "glad/glad.h"
#include <vector>
#include "Texture.h"

class Mesh {
	unsigned int VAO = -1, EBO = -1, VBO = -1;
	unsigned long long vertexOffset;
	
public:
    float* data;
    unsigned int* indices;
    Engine::Texture* texture;
    unsigned int vertexCount, vertexSize;
    int indicesCount, stride;
	Mesh(unsigned int vertexCount, unsigned int vertexSize, int indicesCount);

	void draw() const;

	void addParameter(int location, int size, bool normalized = GL_FALSE);

    void upload();

	[[nodiscard]] bool hasTexture() const;
};
