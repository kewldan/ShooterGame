#pragma once

#include "Frustum.h"
#include "Shader.h"
#include <functional>
#include <memory>

class Minimap {
	int w, h, altitude;
	glm::vec3* pos;
    std::unique_ptr<Engine::Shader> shader;
	unsigned int depth{};
public:
    bool visible = true;
	unsigned int FBO{}, map{};
	// `position` must outlive the minimap (it is read every pass).
	Minimap(const char* shaderName, int width, int height, glm::vec3* position, int altitude);
	~Minimap();

	Minimap(const Minimap&) = delete;
	Minimap& operator=(const Minimap&) = delete;

    // `useFunction` gets the bound map shader and the minimap's ortho frustum for culling.
    void pass(float rotation_y, const std::function<void(Engine::Shader *, const Frustum &)> &useFunction);
};
