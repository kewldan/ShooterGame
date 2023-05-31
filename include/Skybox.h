#pragma once

#include "glad/glad.h"
#include "plog/Log.h"
#include "Mesh.h"
#include "Shader.h"
#include "Camera3D.h"

class Skybox {
	int width, height;
	unsigned int texture;
	Mesh* mesh;
public:
	explicit Skybox(const char* filename);
	~Skybox();

	void draw(Engine::Shader* shader, Engine::Camera3D* camera);

	void bind() const;
};
