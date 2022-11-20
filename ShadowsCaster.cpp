#include "ShadowsCaster.h"

ShadowsCaster::ShadowsCaster(int width, int height, const char* shaderName, glm::vec3 position) {
	w = width;
	h = height;
	glGenFramebuffers(1, &FBO);
	glGenTextures(1, &map);
	glBindTexture(GL_TEXTURE_2D, map);
	glObjectLabelStr(GL_TEXTURE, map, "Texture [Shadow caster]");
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glObjectLabelStr(GL_FRAMEBUFFER, FBO, "FBO [Shadow caster]");
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, map, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	shader = new Shader(shaderName);
	this->position = position;
}

Shader* ShadowsCaster::begin(glm::vec3 cam, float distance) {
	glm::mat4 lightProjection = glm::ortho(-1.0f * distance, 1.0f * distance, -1.0f * distance, 1.0f * distance, 0.1f, 100.f);
	glm::mat4 lightView = glm::lookAt(position+cam,
		cam,
		glm::vec3(0, 1, 0));


	lightSpaceMatrix = lightProjection * lightView;

	glCullFace(GL_FRONT);
	glViewport(0, 0, w, h);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	shader->bind();
	shader->upload("lightSpaceMatrix", lightSpaceMatrix);
	return shader;
}

void ShadowsCaster::end() {
	glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 ShadowsCaster::getLightSpaceMatrix() {
	return lightSpaceMatrix;
}

ShadowsCaster::~ShadowsCaster()
{
	delete shader;
	glDeleteFramebuffers(1, &FBO);
	glDeleteTextures(1, &map);
}

unsigned int ShadowsCaster::getMap() const {
	return map;
}
