#include "ShadowsCaster.h"

#include <cmath>

ShadowsCaster::ShadowsCaster(int width, int height, const char* shaderName, glm::vec3 lightDir, float distance)
	: w(width), h(height), distance(distance), lightDir(glm::normalize(lightDir)) {
	glGenFramebuffers(1, &FBO);
	glGenTextures(1, &map);
	glBindTexture(GL_TEXTURE_2D, map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
		w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	// Sampled through a sampler2DShadow: the hardware does the depth comparison and, with
	// linear filtering, a free 2x2 PCF on top of the one in pass2.frag.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	// Outside the frustum everything is lit.
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, map, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		PLOGE << "Shadow map framebuffer not complete!";
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// A cube of side 2*distance around the centre: the eye sits `distance` behind it (see pass()).
	proj = glm::ortho(-distance, distance, -distance, distance, 0.f, 2.f * distance);

	shader = std::make_unique<Engine::Shader>(shaderName);
}

void ShadowsCaster::pass(glm::vec3 center, const std::function<void(Engine::Shader *)> &useFunction) {
	// Pure rotation into light space (the light looks down -Z there).
	const glm::vec3 up = std::abs(lightDir.y) > 0.99f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
	const glm::mat4 rotation = glm::lookAt(-lightDir, glm::vec3(0), up);

	// Snap the frustum origin to the shadow-map texel grid so the shadow edges do not
	// shimmer while the player (and with him the frustum) moves.
	const float texel = 2.f * distance / static_cast<float>(w);
	glm::vec3 c = glm::vec3(rotation * glm::vec4(center, 1.f));
	c.x = std::floor(c.x / texel) * texel;
	c.y = std::floor(c.y / texel) * texel;

	// Put the centre at z = -distance, in the middle of the [0, 2*distance] depth range.
	view = glm::translate(glm::mat4(1), -c - glm::vec3(0, 0, distance)) * rotation;

	lightSpaceMatrix = proj * view;

	// The map has single-sided walls, so front-face culling would lose their shadows;
	// draw both sides and fight acne with a slope-scaled depth offset instead.
	glDisable(GL_CULL_FACE);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.f, 4.f);
	glViewport(0, 0, w, h);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	shader->bind();
	shader->upload("lightSpaceMatrix", lightSpaceMatrix);

    useFunction(shader.get());

    glDisable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

const glm::mat4& ShadowsCaster::getLightSpaceMatrix() const {
	return lightSpaceMatrix;
}

const glm::vec3& ShadowsCaster::getLightDir() const {
	return lightDir;
}

ShadowsCaster::~ShadowsCaster()
{
	glDeleteFramebuffers(1, &FBO);
	glDeleteTextures(1, &map);
}

unsigned int ShadowsCaster::getMap() const {
	return map;
}
