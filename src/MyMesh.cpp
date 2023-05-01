#include "MyMesh.h"

MyMesh::MyMesh(std::vector<float>* vertices, std::vector<int>* indices, unsigned int vertexSize)
{
	VAO = -1;
	VBO = -1;
	EBO = -1;

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (int)(indices->size() * sizeof(int)), indices->data(),
		GL_STATIC_DRAW);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, (int)(vertices->size() * sizeof(float)), vertices->data(), GL_STATIC_DRAW);

	indicesCount = (int)indices->size();
	stride = (int)(vertexSize * sizeof(float));
	vertexOffset = 0;

	texture = nullptr;
}

void MyMesh::draw() const {
	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, nullptr);
}

void MyMesh::addParameter(int location, int size, bool normalized) {
	glEnableVertexAttribArray(location);
	glVertexAttribPointer(location, size, GL_FLOAT, normalized,
		stride, (void*)(vertexOffset));
	vertexOffset += sizeof(float) * size;
}

bool MyMesh::hasTexture() const
{
	return texture != nullptr;
}
