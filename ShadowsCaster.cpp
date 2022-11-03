#include "ShadowsCaster.h"

ShadowsCaster::ShadowsCaster(int width, int height, const char *shaderName, float z_near, float z_far,
                             glm::vec3 *position, glm::vec3 *lookAt) {
    w = width;
    h = height;
    glGenFramebuffers(1, &FBO);
    glGenTextures(1, &map);
    glBindTexture(GL_TEXTURE_2D, map);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, map, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    shader = new Shader(shaderName);

    glm::mat4 lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, z_near, z_far);
    glm::mat4 lightView = glm::lookAt(*position,
                                      *lookAt,
                                      glm::vec3(0, 1, 0));

    lightSpaceMatrix = lightProjection * lightView;
}

Shader *ShadowsCaster::begin() {
    glCullFace(GL_FRONT);
    glViewport(0, 0, w, h);
    glBindFramebuffer(GL_FRAMEBUFFER, map);
    glClear(GL_DEPTH_BUFFER_BIT);
    shader->bind();
    shader->upload("lightSpaceMatrix", lightSpaceMatrix);
    return shader;
}

void ShadowsCaster::end() {
    shader->unbind();
    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 *ShadowsCaster::getLightSpaceMatrix() {
    return &lightSpaceMatrix;
}

unsigned int ShadowsCaster::getMap() const {
    return map;
}
