#pragma once

#include "glad/glad.h"
#include <vector>
#include "Texture.h"

class MyMesh {
	unsigned int VAO, EBO, VBO;
	unsigned long long vertexOffset;
	
public:
    Engine::Texture* texture;
	int indicesCount, stride;
	MyMesh(std::vector<float>* vertices, std::vector<int>* indices, unsigned int vertexSize);

	void draw() const;

	void addParameter(int location, int size, bool normalized = GL_FALSE);

	[[nodiscard]] bool hasTexture() const;
};
