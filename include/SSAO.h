#pragma once

#include "Shader.h"
#include <memory>
#include "Camera3D.h"

class SSAO {
	int w, h;
	unsigned int ssaoFBO = 0, ssaoBlurFBO = 0;
	unsigned int ssaoColorBuffer = 0;
	unsigned int noiseTexture = 0;
	unsigned int VAO = 0, VBO = 0;
    std::unique_ptr<Engine::Shader> ssaoShader, ssaoBlurShader;
    std::unique_ptr<Engine::UniformBlock> samplesBlock;
public:
    bool visible = true;
	float radius, bias;
	unsigned int ssaoColorBufferBlur = 0;
	SSAO(const char* ssaoShaderPath, const char* ssaoBlurShaderPath, int width, int height);
	~SSAO();

	SSAO(const SSAO&) = delete;
	SSAO& operator=(const SSAO&) = delete;

	void renderSSAOTexture(unsigned int gPosition, unsigned int gNormal, Engine::Camera3D* camera);
	void blurSSAOTexture();
	void resize(int nw, int nh);
};
