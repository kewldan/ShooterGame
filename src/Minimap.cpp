#include "Minimap.h"

Minimap::Minimap(const char *shaderName, int width, int height, glm::vec3 *position, int altitude) {
    w = width;
    h = height;

    glGenTextures(1, &map);
    glBindTexture(GL_TEXTURE_2D, map);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Without a depth attachment the depth test is a no-op and the map is drawn in submission order.
    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);

    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, map, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

    // Must be checked while the FBO is still bound.
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        PLOGE << "Minimap FBO invalid";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    shader = std::make_unique<Engine::Shader>(shaderName);
    shader->bind();
    shader->upload("aTexture", 0);
    this->pos = position;
    this->altitude = altitude;
}

Minimap::~Minimap() {
    glDeleteFramebuffers(1, &FBO);
    glDeleteRenderbuffers(1, &depth);
    glDeleteTextures(1, &map);
}

void Minimap::pass(float rotation_y, const std::function<void(Engine::Shader *)> &useFunction) {
    static const glm::mat4 proj = glm::ortho(-100.f, 100.f, -100.f, 100.f, 0.1f, 300.f);

    if(visible) {
        glm::vec2 rotation = glm::vec2(glm::half_pi<float>(), rotation_y);

        glm::mat4 view = glm::rotate(glm::mat4(1), rotation.x, glm::vec3(1, 0, 0));
        view = glm::rotate(view, rotation.y, glm::vec3(0, 1, 0));
        view = glm::translate(view, -glm::vec3((*pos).x, altitude + (*pos).y, (*pos).z));
        shader->bind();
        shader->upload("proj", proj);
        shader->upload("view", view);
        glActiveTexture(GL_TEXTURE0);

        glViewport(0, 0, w, h);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glClearColor(0.5f, 0.8f, 1.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        useFunction(shader.get());

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}
