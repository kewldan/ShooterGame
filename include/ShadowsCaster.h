#pragma once

#include "glad/glad.h"
#include <functional>
#include "Shader.h"

class ShadowsCaster {
	unsigned int map, FBO;
	int w, h;
    Engine::Shader* shader;
	glm::mat4 lightSpaceMatrix, proj, view;
	glm::vec3 position;
public:
    bool visible = true;

	ShadowsCaster(int width, int height, const char* shaderName, glm::vec3 position, float distance);
	~ShadowsCaster();

    void pass(glm::vec3 cam, const std::function<void(Engine::Shader *)> &useFunction);

	glm::mat4 getLightSpaceMatrix();

	unsigned int getMap() const;
};
