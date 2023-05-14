#pragma once

#include "Shader.h"
#include <functional>
#include "Camera3D.h"

struct Light {
	glm::vec3 pos;
	glm::vec3 color;
};

class GBuffer {
	int w, h;
    unsigned int ssao, shadow;
	Engine::Shader* gShader, * lShader;
public:
	unsigned int FBO, gPosition, gNormal, gAlbedo, rboDepth, VAO, VBO;
	GBuffer(const char* gShaderPath, const char* lShaderPath, int width, int height, unsigned int ssao, unsigned int shadowMap);

	void resize(int nw, int nh);

    void geometryPass(Engine::Camera3D* camera, const std::function<void(Engine::Shader *)> &useFunction);

    void lightingPass(std::vector<Light>* lights, Engine::Camera3D* camera, const std::function<void(Engine::Shader *)> &useFunction);
};