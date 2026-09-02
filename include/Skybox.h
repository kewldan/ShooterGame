#pragma once

#include "glad/glad.h"
#include "plog/Log.h"
#include "Mesh.h"
#include "Shader.h"
#include "Camera3D.h"

class Skybox {
	unsigned int texture = 0;
	Mesh mesh;
public:
	explicit Skybox(const char* filename);
	~Skybox();

	Skybox(const Skybox&) = delete;
	Skybox& operator=(const Skybox&) = delete;

	// `intensity` scales the (linear) cube map colour for the HDR image.
	void draw(Engine::Shader* shader, Engine::Camera3D* camera, float intensity = 1.f);

	void bind() const;
};
