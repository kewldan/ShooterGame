#include "ShadowsCaster.h"

#include <algorithm>
#include <cmath>

ShadowsCaster::ShadowsCaster(int size, const char* shaderName, glm::vec3 lightDir)
	: size(size), lightDir(glm::normalize(lightDir)) {
	glGenFramebuffers(1, &FBO);
	glGenTextures(1, &map);
	glBindTexture(GL_TEXTURE_2D_ARRAY, map);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F,
		size, size, CASCADES, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	// Sampled through a sampler2DArrayShadow: the hardware does the depth comparison and, with
	// linear filtering, a free 2x2 PCF on top of the one in pass2.frag.
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	// Outside a cascade everything is lit (the shader falls through to the next cascade first).
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, map, 0, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		PLOGE << "Shadow map framebuffer not complete!";
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	shader = std::make_unique<Engine::Shader>(shaderName);
}

void ShadowsCaster::computeSplits(float cameraNear, float cameraFar) {
	// Practical split scheme (Zhang et al.): blend of the uniform and the logarithmic distribution.
	const float far = std::min(shadowDistance, cameraFar);
	for (int i = 0; i < CASCADES; i++) {
		const float p = static_cast<float>(i + 1) / CASCADES;
		const float logarithmic = cameraNear * std::pow(far / cameraNear, p);
		const float uniform = cameraNear + (far - cameraNear) * p;
		cascades[i].splitFar = uniform + splitLambda * (logarithmic - uniform);
	}
}

glm::mat4 ShadowsCaster::fitCascade(int i, const glm::mat4 &inverseViewProjection, float cameraNear,
                                    float cameraFar) {
	Cascade &cascade = cascades[i];
	const float sliceNear = i == 0 ? cameraNear : cascades[i - 1].splitFar;
	const float sliceFar = cascade.splitFar;

	// The 8 corners of the slice: the camera frustum corners in world space, moved along the frustum
	// edges (view-space depth is linear along them) to the slice's near and far planes.
	glm::vec3 corners[8];
	for (int c = 0; c < 4; c++) {
		const float x = (c & 1) ? 1.f : -1.f, y = (c & 2) ? 1.f : -1.f;
		glm::vec4 n = inverseViewProjection * glm::vec4(x, y, -1.f, 1.f);
		glm::vec4 f = inverseViewProjection * glm::vec4(x, y, 1.f, 1.f);
		const glm::vec3 nearCorner = glm::vec3(n) / n.w, farCorner = glm::vec3(f) / f.w;
		const float tNear = (sliceNear - cameraNear) / (cameraFar - cameraNear);
		const float tFar = (sliceFar - cameraNear) / (cameraFar - cameraNear);
		corners[c] = glm::mix(nearCorner, farCorner, tNear);
		corners[c + 4] = glm::mix(nearCorner, farCorner, tFar);
	}

	// Enclose the slice in a sphere instead of its light-space AABB: the sphere does not change size
	// when the camera turns, so the ortho box (and with it the texel size) is stable and the shadow
	// edges do not shimmer while looking around.
	glm::vec3 center(0.f);
	for (const auto &corner: corners) center += corner;
	center /= 8.f;
	float radius = 0.f;
	for (const auto &corner: corners) radius = std::max(radius, glm::length(corner - center));
	// Quantise the radius (it only moves while the FOV animates) so the texel size stays put too.
	radius = std::ceil(radius * 16.f) / 16.f;

	// Pure rotation into light space (the light looks down -Z there).
	const glm::vec3 up = std::abs(lightDir.y) > 0.99f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
	const glm::mat4 rotation = glm::lookAt(-lightDir, glm::vec3(0), up);

	// Snap the box origin to the shadow-map texel grid so the shadow edges do not shimmer while the
	// player (and with him the slice) moves.
	const float texel = 2.f * radius / static_cast<float>(size);
	glm::vec3 c = glm::vec3(rotation * glm::vec4(center, 1.f));
	c.x = std::floor(c.x / texel) * texel;
	c.y = std::floor(c.y / texel) * texel;

	// Eye at the sphere centre: the slice spans z in [-radius, radius]; the near plane is pulled back
	// so that casters between the slice and the light (z > radius) still end up in the map.
	const glm::mat4 view = glm::translate(glm::mat4(1), -c) * rotation;
	const float nearPlane = -(radius + casterExtension), farPlane = radius;
	const glm::mat4 proj = glm::ortho(-radius, radius, -radius, radius, nearPlane, farPlane);

	cascade.texelSize = texel;
	cascade.depthRange = farPlane - nearPlane;
	return proj * view;
}

void ShadowsCaster::pass(Engine::Camera3D *camera,
                         const std::function<void(Engine::Shader *, const Frustum &, int)> &useFunction) {
	const glm::mat4 &projection = camera->getProjection();
	// Near/far of a glm::perspective matrix (Engine::Camera3D does not expose them).
	const float cameraNear = projection[3][2] / (projection[2][2] - 1.f);
	const float cameraFar = projection[3][2] / (projection[2][2] + 1.f);
	computeSplits(cameraNear, cameraFar);
	const glm::mat4 inverseViewProjection = glm::inverse(projection * camera->getView());

	// The far cascades change slowly on screen: refresh them every farCascadeInterval frames unless the
	// camera moved or turned enough for their slices to have visibly left the old boxes.
	const glm::vec3 forward = -glm::vec3(glm::row(camera->getView(), 2));
	const bool updateFar = frame % static_cast<unsigned int>(std::max(farCascadeInterval, 1)) == 0
	                       || glm::distance(camera->position, farUpdatePosition) > farUpdateDistance
	                       || glm::dot(forward, farUpdateForward) < 0.996f; // ~5 degrees
	frame++;
	if (updateFar) {
		farUpdatePosition = camera->position;
		farUpdateForward = forward;
	}

	// The map has single-sided walls, so front-face culling would lose their shadows;
	// draw both sides and fight acne with a slope-scaled depth offset instead.
	glDisable(GL_CULL_FACE);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.f, 4.f);
	glViewport(0, 0, size, size);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	shader->bind();

	for (int i = 0; i < CASCADES; i++) {
		if (i >= 2 && !updateFar) continue;

		cascades[i].lightSpaceMatrix = fitCascade(i, inverseViewProjection, cameraNear, cameraFar);
		const Frustum frustum(cascades[i].lightSpaceMatrix);

		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, map, 0, i);
		glClear(GL_DEPTH_BUFFER_BIT);
		shader->upload("lightSpaceMatrix", cascades[i].lightSpaceMatrix);

		useFunction(shader.get(), frustum, i);
	}

	glDisable(GL_POLYGON_OFFSET_FILL);
	glEnable(GL_CULL_FACE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

const ShadowsCaster::Cascade& ShadowsCaster::getCascade(int i) const {
	ASSERT("Cascade index out of range", i >= 0 && i < CASCADES);
	return cascades[i];
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
