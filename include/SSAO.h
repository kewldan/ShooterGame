#pragma once

#include "Shader.h"
#include <random>
#include "Camera3D.h"

float lerp(float a, float b, float f);

class SSAO {
	int w, h;
	unsigned int ssaoFBO, ssaoBlurFBO;
	unsigned int ssaoColorBuffer;
	unsigned int noiseTexture;
	unsigned int VAO, VBO;
    Engine::Shader* ssaoShader, * ssaoBlurShader;
	std::uniform_real_distribution<GLfloat> randomFloats; // generates random floats between 0.0 and 1.0
	std::default_random_engine generator;
	glm::vec4 kernel[24];
	glm::vec3 noise[16];
    Engine::UniformBlock* samplesBlock;
public:
    bool visible = true;
	float radius, bias;
	unsigned int ssaoColorBufferBlur;
	SSAO(const char* ssaoShaderPath, const char* ssaoBlurShaderPath, int width, int height);
	void renderSSAOTexture(unsigned int gPosition, unsigned int gNormal, Engine::Camera3D* camera);
	void blurSSAOTexture();
	void resize(int nw, int nh);
};
