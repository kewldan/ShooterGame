#include "ShadowsCaster.h"

ShadowsCaster::ShadowsCaster(int width, int height, const char* shaderName, glm::vec3 position, float distance) {
	w = width;
	h = height;
	glGenFramebuffers(1, &FBO);
	glGenTextures(1, &map);
	glBindTexture(GL_TEXTURE_2D, map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, map, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	proj = glm::ortho(-distance, distance, -distance, distance, 5.f, 50.f);

	shader = new Engine::Shader(shaderName, true);
}

Engine::Shader* ShadowsCaster::begin(glm::vec3 cam) {
	const static glm::vec2 rotation = glm::vec2(1.1f, 0.4f);

	view = glm::mat4(1);
	view = glm::rotate(view, rotation.x, glm::vec3(1, 0, 0));
	view = glm::rotate(view, rotation.y, glm::vec3(0, 1, 0));
	view = glm::translate(view, -glm::vec3(cam.x + 5, 25, cam.z - 2));

	lightSpaceMatrix = proj * view;

	glDisable(GL_CULL_FACE);
	glViewport(0, 0, w, h);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	shader->bind();
	shader->upload("lightSpaceMatrix", lightSpaceMatrix);
	return shader;
}

void ShadowsCaster::end() {
	glEnable(GL_CULL_FACE);
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
