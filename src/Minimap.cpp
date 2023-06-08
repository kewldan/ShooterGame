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

    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, map, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        PLOGE << "Minimap FBO invalid";
    }

    shader = new Engine::Shader(shaderName);
    this->pos = position;
    this->altitude = altitude;
}

Minimap::~Minimap() {
    delete shader;
    glDeleteFramebuffers(1, &FBO);
    glDeleteTextures(1, &map);
}

void Minimap::pass(float rotation_y, const std::function<void(Engine::Shader *)> &useFunction) {
    static const glm::mat4 proj = glm::ortho(-100.f, 100.f, -100.f, 100.f, 0.1f, 300.f);

    if(visible) {
        glm::vec2 rotation = glm::vec2(1.57f, rotation_y);

        static glm::mat4 view;
        view = glm::rotate(glm::mat4(1), rotation.x, glm::vec3(1, 0, 0));
        view = glm::rotate(view, rotation.y, glm::vec3(0, 1, 0));
        view = glm::translate(view, -glm::vec3((*pos).x, altitude + (*pos).y, (*pos).z));
        shader->bind();
        shader->upload("proj", proj);
        shader->upload("view", view);
        shader->upload("aTexture", 0);
        glActiveTexture(GL_TEXTURE0);

        glViewport(0, 0, w, h);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glClearColor(0.5, 0.8, 1, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        useFunction(shader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}
