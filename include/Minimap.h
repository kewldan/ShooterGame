#pragma once

#include "Shader.h"
#include <functional>

class Minimap {
	int w, h, altitude;
	glm::vec3* pos;
    Engine::Shader* shader;
public:
	unsigned int FBO{}, map{};
	Minimap(const char* shaderName, int width, int height, glm::vec3* position, int altitude);
	~Minimap();
    void pass(float rotation_y, const std::function<void(Engine::Shader *)> &useFunction);
};
