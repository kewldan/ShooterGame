#ifndef SHOOTERGAME_SSAO_H
#define SHOOTERGAME_SSAO_H

#include "Shader.h"
#include <random>

class SSAO {
	int w, h;
	unsigned int ssaoFBO, ssaoBlurFBO;
	unsigned int ssaoColorBuffer;
	unsigned int noiseTexture;
	unsigned int VAO, VBO;
	Shader* ssaoShader, * ssaoBlurShader;
	std::uniform_real_distribution<GLfloat> randomFloats; // generates random floats between 0.0 and 1.0
	std::default_random_engine generator;
	std::vector<glm::vec3> ssaoKernel;
	std::vector<glm::vec3> ssaoNoise;
public:
	unsigned int ssaoColorBufferBlur;
	SSAO(const char* ssaoShaderPath, const char* ssaoBlurShaderPath, int width, int height);
	void renderSSAOTexture(unsigned int gPosition, unsigned int gNormal, glm::mat4 proj);
	void blurSSAOTexture();
};

#endif