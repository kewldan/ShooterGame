#include "SSAO.h"

#include <random>

namespace {
	constexpr int KERNEL_SIZE = 24; // must match `samples[24]` in ssao.frag

	float lerp(float a, float b, float f)
	{
		return a + f * (b - a);
	}
}

SSAO::SSAO(const char* ssaoShaderPath, const char* ssaoBlurShaderPath, int width, int height)
{
	w = width;
	h = height;

	glGenFramebuffers(1, &ssaoFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	// SSAO color message
	glGenTextures(1, &ssaoColorBuffer);
	glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, w, h, 0, GL_RED, GL_FLOAT, nullptr);
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
	glGenTextures(1, &ssaoColorBufferBlur);
	glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, w, h, 0, GL_RED, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		PLOGE << "SSAO Blur Framebuffer not complete!";
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// generate sample kernel
	// ----------------------
	std::uniform_real_distribution<GLfloat> randomFloats(0.f, 1.f); // generates random floats between 0.0 and 1.0
	std::default_random_engine generator;
	glm::vec4 kernel[KERNEL_SIZE]; // vec4 so the layout matches std140 `vec3 samples[]` (16 byte stride)
	for (unsigned int i = 0; i < KERNEL_SIZE; ++i)
	{
		glm::vec4 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator), 0);
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = i / (float) KERNEL_SIZE;

		// scale samples s.t. they're more aligned to center of kernel
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		kernel[i] = sample;
	}

	// generate noise texture
	// ----------------------
	glm::vec3 noise[16];
	for (auto & i : noise)
	{
		i = glm::normalize(glm::vec3(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f));
	}
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, noise);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	ssaoShader = std::make_unique<Engine::Shader>(ssaoShaderPath);
	ssaoBlurShader = std::make_unique<Engine::Shader>(ssaoBlurShaderPath);
	ssaoShader->bindUniformBlock("SamplesUniform");
	samplesBlock = std::make_unique<Engine::UniformBlock>(static_cast<unsigned int>(sizeof(kernel)));
	samplesBlock->add(static_cast<unsigned int>(sizeof(kernel)), kernel);

	// Constant uniforms: upload once, not every frame.
	ssaoShader->bind();
	ssaoShader->upload("gPosition", 0);
	ssaoShader->upload("gNormal", 1);
	ssaoShader->upload("texNoise", 2);
	ssaoShader->upload("noiseScale", glm::vec2(w / 4.f, h / 4.f));
	ssaoBlurShader->bind();
	ssaoBlurShader->upload("ssaoInput", 0);

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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<const void*>(3 * sizeof(float)));

	radius = 0.5f;
	bias = 0.075f;
}

SSAO::~SSAO()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteTextures(1, &noiseTexture);
	glDeleteTextures(1, &ssaoColorBufferBlur);
	glDeleteTextures(1, &ssaoColorBuffer);
	glDeleteFramebuffers(1, &ssaoBlurFBO);
	glDeleteFramebuffers(1, &ssaoFBO);
}

void SSAO::renderSSAOTexture(unsigned int gPosition, unsigned int gNormal, Engine::Camera3D* camera)
{
    if(visible) {
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
        glClear(GL_COLOR_BUFFER_BIT);

        {
            ssaoShader->bind();
            ssaoShader->upload("proj", camera->getProjection());
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
            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void SSAO::blurSSAOTexture()
{
    if(visible) {
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
        glClear(GL_COLOR_BUFFER_BIT);

        {
            ssaoBlurShader->bind();

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
        }

        {
            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void SSAO::resize(int nw, int nh)
{
	w = nw;
	h = nh;

	glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, w, h, 0, GL_RED, GL_FLOAT, nullptr);

	glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, w, h, 0, GL_RED, GL_FLOAT, nullptr);

	ssaoShader->bind();
	ssaoShader->upload("noiseScale", glm::vec2(w / 4.f, h / 4.f));
}
