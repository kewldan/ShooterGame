#include "SSAO.h"

float ourLerp(float a, float b, float f)
{
	return a + f * (b - a);
}

SSAO::SSAO(const char* ssaoShaderPath, const char* ssaoBlurShaderPath, int width, int height)
{
	w = width;
	h = height;

	glGenFramebuffers(1, &ssaoFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	glObjectLabelBuild(GL_FRAMEBUFFER, ssaoFBO, "FBO", "SSAO");
	// SSAO color buffer
	glGenTextures(1, &ssaoColorBuffer);
	glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
	glObjectLabelBuild(GL_TEXTURE, ssaoColorBuffer, "Texture", "SSAO Color");
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		PLOGE << "SSAO Framebuffer not complete!";
	// and blur stage

	glGenFramebuffers(1, &ssaoBlurFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
	glObjectLabelBuild(GL_FRAMEBUFFER, ssaoBlurFBO, "FBO", "SSAO Blur");
	glGenTextures(1, &ssaoColorBufferBlur);
	glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
	glObjectLabelBuild(GL_TEXTURE, ssaoColorBufferBlur, "Texture", "SSAO Blur color");
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		PLOGE << "SSAO Blur Framebuffer not complete!";
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// generate sample kernel
	// ----------------------
	randomFloats = std::uniform_real_distribution<GLfloat>(0.f, 1.f);
	generator = std::default_random_engine();
	for (unsigned int i = 0; i < 64; ++i)
	{
		glm::vec4 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator), 0);
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / 64.0f;

		// scale samples s.t. they're more aligned to center of kernel
		scale = ourLerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		kernel[i] = sample;
	}

	// generate noise texture
	// ----------------------
	for (unsigned int i = 0; i < 16; i++)
	{
		noise[i] = glm::normalize(glm::vec3(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f));
	}
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glObjectLabelBuild(GL_TEXTURE, noiseTexture, "Texture", "SSAO noise");
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, noise);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	ssaoShader = new Shader(ssaoShaderPath);
	ssaoBlurShader = new Shader(ssaoBlurShaderPath);
	ssaoShader->bindUniformBlock("SamplesUniform");
	samplesBlock = new UniformBlock(64 * sizeof(glm::vec4));
	samplesBlock->add(64 * sizeof(glm::vec4), kernel);

	float quadVertices[] = {
		// positions        // texture Coords
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};
	// setup plane VAO
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	radius = 0.5f;
	bias = 0.075f;
}

void SSAO::renderSSAOTexture(unsigned int gPosition, unsigned int gNormal, Camera* camera)
{
	rmt_ScopedCPUSample(SSAO_RenderTexture, 0)
		glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	glClear(GL_COLOR_BUFFER_BIT);

	{
		rmt_ScopedCPUSample(SSAO_Preparing, 0)
			ssaoShader->bind();
		ssaoShader->upload("proj", camera->getPerspective());
		ssaoShader->upload("gPosition", 0);
		ssaoShader->upload("gNormal", 1);
		ssaoShader->upload("texNoise", 2);
		ssaoShader->upload("noiseScale", glm::vec2(w / 4.f, h / 4.f));
		ssaoShader->upload("radius", radius);
		ssaoShader->upload("bias", bias);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);
	}

	{
		rmt_ScopedCPUSample(SSAO_Draw, 0)
			glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSAO::blurSSAOTexture()
{
	rmt_ScopedCPUSample(SSAO_Bluring, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
	glClear(GL_COLOR_BUFFER_BIT);

	{
		rmt_ScopedCPUSample(SSAO_Blur_Preparing, 0)
			ssaoBlurShader->bind();
		ssaoBlurShader->upload("ssaoInput", 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
	}

	{
		rmt_ScopedCPUSample(SSAO_Draw, 0)
			glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSAO::resize(int nw, int nh)
{
	w = nw;
	h = nh;

	glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_FLOAT, NULL);
}
