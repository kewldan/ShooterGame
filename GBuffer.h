#ifndef SHOOTERGAME_GBUFFER_H
#define SHOOTERGAME_GBUFFER_H

#include "Shader.h"

class GBuffer {
	unsigned int FBO, gPosition, gNormal, gAlbedo, rboDepth, VAO, VBO;
	int* w, *h;
	Shader* gShader, * lShader;
public:
	GBuffer(const char* gShaderPath, const char* lShaderPath, int* width, int* height);

	Shader* beginGeometryPass(glm::mat4 proj, glm::mat4 view);
	void endGeometryPass();
};

#endif