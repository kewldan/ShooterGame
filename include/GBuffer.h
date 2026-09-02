#pragma once

#include "Shader.h"
#include <functional>
#include <memory>
#include "Camera3D.h"
#include "Frustum.h"
#include "ScreenQuad.h"

// The deferred geometry pass and the fullscreen sun/ambient lighting pass. Everything in the G-buffer is
// view space (see pass1.vert). The albedo target is sRGB: pass1.frag writes the texture samples as they
// are (sRGB bytes, no encoding since GL_FRAMEBUFFER_SRGB stays off) and every pass that samples it gets
// linear light back from the hardware decode for free. Point lights are not part of the lighting pass,
// they are light volumes (PointLights) added on top of its output.
class GBuffer {
	int w, h;
    unsigned int ssao, shadow;
	std::unique_ptr<Engine::Shader> gShader, lShader;
	ScreenQuad quad;
public:
	unsigned int FBO = 0, gPosition = 0, gNormal = 0, gAlbedo = 0, rboDepth = 0;
	// `shadowMap` is a GL_TEXTURE_2D_ARRAY with one layer per shadow cascade (see ShadowsCaster).
	GBuffer(const char* gShaderPath, const char* lShaderPath, int width, int height, unsigned int ssao, unsigned int shadowMap);
	~GBuffer();

	GBuffer(const GBuffer&) = delete;
	GBuffer& operator=(const GBuffer&) = delete;

	// Re-allocates the targets in place: the texture and renderbuffer names stay the same, so whoever
	// attached or bound them (the HDR target shares rboDepth) needs no update.
	void resize(int nw, int nh);

    // `useFunction` gets the bound geometry shader and the camera frustum for culling.
    void geometryPass(Engine::Camera3D* camera, const std::function<void(Engine::Shader *, const Frustum &)> &useFunction);

    // Draws the lit scene (sun, ambient, SSAO, shadows) as a fullscreen quad into whatever framebuffer is
    // bound; `useFunction` gets the bound lighting shader to upload the sun and shadow uniforms.
    void lightingPass(const std::function<void(Engine::Shader *)> &useFunction);
};
