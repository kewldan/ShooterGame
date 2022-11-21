#ifndef SHOOTERGAME_GBUFFER_H
#define SHOOTERGAME_GBUFFER_H

#include "Shader.h"

struct Light {
	glm::vec3 pos;
	glm::vec3 color;
};

class GBuffer {
	int w, h;
	Shader* gShader, * lShader;
public:
	unsigned int FBO, gPosition, gNormal, gAlbedo, rboDepth, VAO, VBO;
	GBuffer(const char* gShaderPath, const char* lShaderPath, int width, int height);

	void resize(int nw, int nh);

	Shader* beginGeometryPass(Camera* camera);
	void endGeometryPass();

	Shader* beginLightingPass(std::vector<Light>* lights, glm::vec3 camera_pos, unsigned int ssao);
	void endLightingPass();
};

#endif