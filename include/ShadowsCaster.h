#pragma once

#include "glad/glad.h"
#include <functional>
#include <memory>
#include "Shader.h"

class ShadowsCaster {
	unsigned int map = 0, FBO = 0;
	int w, h;
    std::unique_ptr<Engine::Shader> shader;
	glm::mat4 lightSpaceMatrix{}, proj{}, view{};
public:
    bool visible = true;

	// The light follows the camera (see pass()), `distance` is the half-size of the ortho frustum.
	ShadowsCaster(int width, int height, const char* shaderName, float distance);
	~ShadowsCaster();

	ShadowsCaster(const ShadowsCaster&) = delete;
	ShadowsCaster& operator=(const ShadowsCaster&) = delete;

    void pass(glm::vec3 cam, const std::function<void(Engine::Shader *)> &useFunction);

	[[nodiscard]] const glm::mat4& getLightSpaceMatrix() const;

	[[nodiscard]] unsigned int getMap() const;
};
