#pragma once

#include "glad/glad.h"
#include <memory>
#include <vector>
#include "Texture.h"

class Mesh {
	unsigned int VAO = 0, EBO = 0, VBO = 0; // 0 = not uploaded (GL never hands out 0)
	unsigned long long vertexOffset = 0;

public:
	// CPU-side copies; released after upload().
	std::vector<float> data;
	std::vector<unsigned int> indices;
	std::unique_ptr<Engine::Texture> texture;
	unsigned int vertexCount, vertexSize;
	int indicesCount, stride;

	Mesh(unsigned int vertexCount, unsigned int vertexSize, int indicesCount);
	~Mesh();

	// Owns GL objects: movable, not copyable.
	Mesh(const Mesh &) = delete;
	Mesh &operator=(const Mesh &) = delete;
	Mesh(Mesh &&other) noexcept;
	Mesh &operator=(Mesh &&other) noexcept;

	void draw() const;

	void addParameter(int location, int size, bool normalized = GL_FALSE);

	void upload();

	[[nodiscard]] bool hasTexture() const;

	[[nodiscard]] bool isUploaded() const;
};
