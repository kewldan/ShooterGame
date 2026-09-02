#pragma once

#include "ScreenQuad.h"
#include "Shader.h"
#include <glm/glm.hpp>
#include <memory>

// The HDR half of the frame and what turns it into the picture on screen. Everything lit (the lighting
// pass, light volumes, skybox, sun, effects, view-model) is drawn into an RGBA16F target in linear light
// (sun about 4, sky 1-2, tracers and muzzle flash well above 1). render() then:
//   1. bloom: soft-knee threshold -> 13-tap downsample chain (BLOOM_MIPS, half resolution each) ->
//      tent-filter upsample, added progressively back up the chain;
//   2. tone mapping (ACES fitted / Uncharted 2) with exposure, plus the sRGB gamma, into an RGBA8 target
//      whose alpha holds the luma;
//   3. FXAA 3.11 (quality preset 12) from that target into the default framebuffer, or, with FXAA off,
//      step 2 writes to the default framebuffer directly.
// The HDR target shares the depth renderbuffer of the G-buffer, so forward passes are depth tested
// against the scene without a blit; that renderbuffer is resized by GBuffer::resize.
class PostProcess {
public:
    static constexpr int BLOOM_MIPS = 5;

private:
    int w, h;
    unsigned int hdrFBO = 0, hdrColor = 0;
    unsigned int ldrFBO = 0, ldrColor = 0;
    unsigned int bloomFBO = 0, bloomMips[BLOOM_MIPS]{};
    glm::ivec2 mipSizes[BLOOM_MIPS]{};
    std::unique_ptr<Engine::Shader> bloomDownShader, bloomUpShader, tonemapShader, fxaaShader;
    ScreenQuad quad;

    void allocate();

    void renderBloom();

public:
    // Settings (the Graphics tree exposes them).
    float exposure = 0.5f;         // a sunlit wall (albedo 0.5, sun 4) lands near display white
    int tonemapper = 0;            // 0 = ACES fitted, 1 = Uncharted 2, 2 = none (clamp)
    bool bloom = true;
    float bloomStrength = 0.06f;   // fraction of the bloom chain added to the image
    float bloomThreshold = 1.2f;   // linear luminance where bloom starts
    float bloomKnee = 0.5f;        // width of the soft threshold below bloomThreshold
    bool fxaa = true;

    // `depthRenderbuffer` is the G-buffer's depth (GBuffer::rboDepth); it must outlive this object.
    PostProcess(int width, int height, unsigned int depthRenderbuffer);

    ~PostProcess();

    PostProcess(const PostProcess &) = delete;

    PostProcess &operator=(const PostProcess &) = delete;

    void resize(int width, int height);

    // Binds the HDR target (viewport set, colour cleared; the depth is the G-buffer's, already filled).
    void beginHdr() const;

    [[nodiscard]] unsigned int getHdrFBO() const;

    [[nodiscard]] unsigned int getHdrTexture() const;

    // Bloom + tone mapping (+ FXAA) from the HDR target into the default framebuffer, which is left
    // bound with the window viewport for the HUD.
    void render();
};
