#pragma once

#include "Shader.h"

class Minimap {
	int w, h, altitude;
	glm::vec3* pos;
    Engine::Shader* shader;
public:
	unsigned int FBO{}, map{};
	Minimap(const char* shaderName, int width, int height, glm::vec3* position, int altitude);
	~Minimap();
    Engine::Shader* begin(float rotation_y);
	static void end();
};
