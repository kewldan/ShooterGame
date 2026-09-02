#include "PostProcess.h"

#include <algorithm>

namespace {
    void setupColorTexture(unsigned int texture, GLint internalFormat, int w, int h, GLenum format, GLenum type) {
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, format, type, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
}

PostProcess::PostProcess(int width, int height, unsigned int depthRenderbuffer) : w(width), h(height) {
    glGenFramebuffers(1, &hdrFBO);
    glGenFramebuffers(1, &ldrFBO);
    glGenFramebuffers(1, &bloomFBO);
    glGenTextures(1, &hdrColor);
    glGenTextures(1, &ldrColor);
    glGenTextures(BLOOM_MIPS, bloomMips);
    allocate();

    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColor, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        PLOGE << "HDR framebuffer not complete!";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, ldrFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ldrColor, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        PLOGE << "LDR framebuffer not complete!";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomMips[0], 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        PLOGE << "Bloom framebuffer not complete!";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    bloomDownShader = std::make_unique<Engine::Shader>("bloomDown");
    bloomUpShader = std::make_unique<Engine::Shader>("bloomUp");
    tonemapShader = std::make_unique<Engine::Shader>("tonemap");
    fxaaShader = std::make_unique<Engine::Shader>("fxaa");
    // Sampler bindings never change.
    bloomDownShader->bind();
    bloomDownShader->upload("source", 0);
    bloomUpShader->bind();
    bloomUpShader->upload("source", 0);
    tonemapShader->bind();
    tonemapShader->upload("hdr", 0);
    tonemapShader->upload("bloom", 1);
    fxaaShader->bind();
    fxaaShader->upload("source", 0);
}

PostProcess::~PostProcess() {
    glDeleteTextures(BLOOM_MIPS, bloomMips);
    glDeleteTextures(1, &ldrColor);
    glDeleteTextures(1, &hdrColor);
    glDeleteFramebuffers(1, &bloomFBO);
    glDeleteFramebuffers(1, &ldrFBO);
    glDeleteFramebuffers(1, &hdrFBO);
}

void PostProcess::allocate() {
    setupColorTexture(hdrColor, GL_RGBA16F, w, h, GL_RGBA, GL_FLOAT);
    setupColorTexture(ldrColor, GL_RGBA8, w, h, GL_RGBA, GL_UNSIGNED_BYTE);
    glm::ivec2 size(w, h);
    for (int i = 0; i < BLOOM_MIPS; i++) {
        size = glm::max(size / 2, glm::ivec2(1));
        mipSizes[i] = size;
        setupColorTexture(bloomMips[i], GL_RGBA16F, size.x, size.y, GL_RGBA, GL_FLOAT);
    }
}

void PostProcess::resize(int width, int height) {
    w = std::max(width, 1);
    h = std::max(height, 1);
    allocate();
}

void PostProcess::beginHdr() const {
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glViewport(0, 0, w, h);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);
}

unsigned int PostProcess::getHdrFBO() const {
    return hdrFBO;
}

unsigned int PostProcess::getHdrTexture() const {
    return hdrColor;
}

void PostProcess::renderBloom() {
    glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO);
    glActiveTexture(GL_TEXTURE0);

    // Down: HDR -> mip 0 (with the threshold) -> mip 1 -> ... each half the size of the previous one.
    bloomDownShader->bind();
    bloomDownShader->upload("exposure", exposure);
    bloomDownShader->upload("threshold", bloomThreshold);
    bloomDownShader->upload("knee", bloomKnee);
    unsigned int source = hdrColor;
    glm::ivec2 sourceSize(w, h);
    for (int i = 0; i < BLOOM_MIPS; i++) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomMips[i], 0);
        glViewport(0, 0, mipSizes[i].x, mipSizes[i].y);
        glBindTexture(GL_TEXTURE_2D, source);
        bloomDownShader->upload("sourceTexel", 1.f / glm::vec2(sourceSize));
        bloomDownShader->upload("prefilter", i == 0 ? 1 : 0);
        quad.draw();
        source = bloomMips[i];
        sourceSize = mipSizes[i];
    }

    // Up: every mip gets the tent-filtered next smaller one added, so mip 0 ends up as the sum of all
    // blur radii (the wide, soft halo around very bright spots).
    bloomUpShader->bind();
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    for (int i = BLOOM_MIPS - 2; i >= 0; i--) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomMips[i], 0);
        glViewport(0, 0, mipSizes[i].x, mipSizes[i].y);
        glBindTexture(GL_TEXTURE_2D, bloomMips[i + 1]);
        bloomUpShader->upload("sourceTexel", 1.f / glm::vec2(mipSizes[i + 1]));
        quad.draw();
    }
    glDisable(GL_BLEND);
}

void PostProcess::render() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    if (bloom) {
        renderBloom();
    }

    // Tone mapping: into the LDR target when FXAA still has to read it, straight to the screen otherwise.
    glBindFramebuffer(GL_FRAMEBUFFER, fxaa ? ldrFBO : 0);
    glViewport(0, 0, w, h);
    tonemapShader->bind();
    tonemapShader->upload("exposure", exposure);
    tonemapShader->upload("tonemapper", tonemapper);
    tonemapShader->upload("bloomStrength", bloom ? bloomStrength : 0.f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrColor);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloomMips[0]);
    quad.draw();

    if (fxaa) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        fxaaShader->bind();
        fxaaShader->upload("invSize", 1.f / glm::vec2(w, h));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ldrColor);
        quad.draw();
    }

    glEnable(GL_DEPTH_TEST);
}
