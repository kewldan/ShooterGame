#pragma once

#include "glad/glad.h"
#include <functional>
#include <memory>
#include "Camera3D.h"
#include "Frustum.h"
#include "Shader.h"

// Directional (sun) cascaded shadow maps: the camera frustum is cut into CASCADES depth slices and each
// slice gets its own orthographic light frustum and layer of a depth texture array. Nearby geometry thus
// gets far more shadow texels than distant geometry. The lighting pass (pass2.frag) picks the cascade
// from the view-space depth of the pixel.
class ShadowsCaster {
public:
	static constexpr int CASCADES = 4;

	// One shadow map layer.
	struct Cascade {
		glm::mat4 lightSpaceMatrix{1.f}; // world -> light clip space of the layer as last rendered
		float splitFar = 0.f;             // view-space depth where the cascade ends
		float texelSize = 0.f;            // world units per shadow-map texel (for the shader bias)
		float depthRange = 0.f;           // world units covered by depth 0..1 (for the shader bias)
	};

private:
	unsigned int map = 0, FBO = 0;
	int size;
	glm::vec3 lightDir;
	std::unique_ptr<Engine::Shader> shader;
	Cascade cascades[CASCADES];
	unsigned int frame = 0;
	glm::vec3 farUpdatePosition{1e30f};
	glm::vec3 farUpdateForward{0.f};

	void computeSplits(float cameraNear, float cameraFar);

	// Fits cascade `i` around its camera frustum slice and returns the light view-projection.
	glm::mat4 fitCascade(int i, const glm::mat4 &inverseViewProjection, float cameraNear, float cameraFar);

public:
	bool visible = true;
	// How far from the camera shadows are rendered (the far plane of the last cascade), world units.
	float shadowDistance = 200.f;
	// Practical split scheme: 0 = uniform splits, 1 = logarithmic.
	float splitLambda = 0.75f;
	// How far behind a slice (towards the light) casters are still rendered, world units.
	float casterExtension = 100.f;
	// Cascades 2.. are re-rendered only every N frames, or when the camera moved or turned noticeably.
	int farCascadeInterval = 2;
	float farUpdateDistance = 1.f;

	// `size` is the resolution of every layer, `lightDir` the direction the light travels in (world space).
	ShadowsCaster(int size, const char* shaderName, glm::vec3 lightDir);
	~ShadowsCaster();

	ShadowsCaster(const ShadowsCaster&) = delete;
	ShadowsCaster& operator=(const ShadowsCaster&) = delete;

	// Renders the shadow map layers for the current camera. `useFunction` is called once per rendered
	// cascade with the depth shader (bound, `lightSpaceMatrix` uploaded), the light frustum of that
	// cascade for culling and the cascade index.
	void pass(Engine::Camera3D *camera,
	          const std::function<void(Engine::Shader *, const Frustum &, int)> &useFunction);

	[[nodiscard]] const Cascade& getCascade(int i) const;

	[[nodiscard]] const glm::vec3& getLightDir() const;

	// The GL_TEXTURE_2D_ARRAY depth texture, one layer per cascade (sampler2DArrayShadow).
	[[nodiscard]] unsigned int getMap() const;
};
