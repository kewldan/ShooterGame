#pragma once

#include "glad/glad.h"
#include <functional>
#include <memory>
#include "Shader.h"

// Directional (sun) shadow map: an orthographic box of half-size `distance` centred on the point
// handed to pass(), looking along `lightDir`.
class ShadowsCaster {
	unsigned int map = 0, FBO = 0;
	int w, h;
	float distance;
	glm::vec3 lightDir;
    std::unique_ptr<Engine::Shader> shader;
	glm::mat4 lightSpaceMatrix{}, proj{}, view{};
public:
    bool visible = true;

	// `lightDir` is the direction the light travels in (world space), `distance` is the half-size of the ortho frustum.
	ShadowsCaster(int width, int height, const char* shaderName, glm::vec3 lightDir, float distance);
	~ShadowsCaster();

	ShadowsCaster(const ShadowsCaster&) = delete;
	ShadowsCaster& operator=(const ShadowsCaster&) = delete;

	// `center` is the world-space point the shadow frustum is centred on (the player).
    void pass(glm::vec3 center, const std::function<void(Engine::Shader *)> &useFunction);

	[[nodiscard]] const glm::mat4& getLightSpaceMatrix() const;

	[[nodiscard]] const glm::vec3& getLightDir() const;

	[[nodiscard]] unsigned int getMap() const;
};
