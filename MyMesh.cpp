#include "MyMesh.h"

MyMesh::MyMesh(std::vector<float>* vertices, std::vector<int>* indices, unsigned int vertexSize) {
	VAO = 0;
	VBO = 0;
	EBO = 0;

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
}

void MyMesh::draw() const {
	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, nullptr);
}

MyMesh::~MyMesh() {
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteVertexArrays(1, &VAO);
}

void MyMesh::addParameter(int location, int size, bool normalized) {
	glEnableVertexAttribArray(location);
	glVertexAttribPointer(location, size, GL_FLOAT, normalized,
		stride, (void*)(vertexOffset));
	vertexOffset += sizeof(float) * size;
}

void MyMesh::addParameter(int location, int size) {
	addParameter(location, size, GL_FALSE);
}
