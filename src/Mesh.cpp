#include "Mesh.h"

void Mesh::draw() const {
    ASSERT("Draw non uploaded mesh", VAO > 0 && EBO > 0 && VBO > 0);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, nullptr);
}

void Mesh::addParameter(int location, int size, bool normalized) {
    ASSERT("Add parameter to non uploaded mesh", VAO > 0 && EBO > 0 && VBO > 0);
	glEnableVertexAttribArray(location);
	glVertexAttribPointer(location, size, GL_FLOAT, normalized,
		stride, (void*)(vertexOffset));
	vertexOffset += sizeof(float) * size;
}

bool Mesh::hasTexture() const
{
	return texture != nullptr;
}

void Mesh::upload() {
    ASSERT("Mesh already uploaded to GPU memory", VAO == -1 && EBO == -1 && VBO == -1);
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(unsigned int), indices,
                 GL_STATIC_DRAW);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * vertexSize * sizeof(float), data, GL_STATIC_DRAW);
}

Mesh::Mesh(unsigned int vertexCount, unsigned int vertexSize, int indicesCount) : vertexCount(vertexCount), vertexSize(vertexSize), indicesCount(indicesCount) {
    ASSERT("Indices count <= 0", indicesCount > 0);
    stride = static_cast<int>(vertexSize * sizeof(float));
    vertexOffset = 0;

    data = new float[vertexCount * vertexSize];
    indices = new unsigned int[indicesCount];

    texture = nullptr;
}
