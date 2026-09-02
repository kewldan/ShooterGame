#pragma once

#include "Shader.h"
#include <functional>
#include <memory>
#include <vector>
#include "Camera3D.h"
#include "Frustum.h"

// NOTE: the lighting pass works in view space (see pass1.vert), so light positions are view-space too.
struct Light {
	glm::vec3 pos;
	glm::vec3 color;
};

class GBuffer {
	int w, h;
    unsigned int ssao, shadow;
	std::unique_ptr<Engine::Shader> gShader, lShader;
public:
	unsigned int FBO = 0, gPosition = 0, gNormal = 0, gAlbedo = 0, rboDepth = 0, VAO = 0, VBO = 0;
	// `shadowMap` is a GL_TEXTURE_2D_ARRAY with one layer per shadow cascade (see ShadowsCaster).
	GBuffer(const char* gShaderPath, const char* lShaderPath, int width, int height, unsigned int ssao, unsigned int shadowMap);
	~GBuffer();

	GBuffer(const GBuffer&) = delete;
	GBuffer& operator=(const GBuffer&) = delete;

	void resize(int nw, int nh);

    // `useFunction` gets the bound geometry shader and the camera frustum for culling.
    void geometryPass(Engine::Camera3D* camera, const std::function<void(Engine::Shader *, const Frustum &)> &useFunction);

    void lightingPass(const std::vector<Light>& lights, const std::function<void(Engine::Shader *)> &useFunction);
};
