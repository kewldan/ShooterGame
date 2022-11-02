#include "Shader.h"
#include <cstdlib>
#include "Window.h"

#include <plog/Log.h>
#include "plog/Initializers/RollingFileInitializer.h"
#include "Model.h"
#include "Camera.h"
#include "Texture.h"

int main() {
    std::remove("latest.log");
    plog::init(plog::debug, "latest.log");
    PLOG_INFO << "Logger initialized";
    auto *window = new Window();

    auto *shader = new Shader("main");
    auto *depthShader = new Shader("depth");

    auto *sniperRifle = new Model("./data/meshes/sniper.obj");
    sniperRifle->scale *= 2.f;
    auto *map = new Model("./data/meshes/map.obj");
    map->scale *= 10.f;
    auto *cube = new Model("./data/meshes/cube.obj");

    auto *sniperTexture = new Texture("./data/textures/sniper.png");
    auto *mapTexture = new Texture("./data/textures/palette.png");

    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);

    const unsigned int SHADOW_WIDTH = 256, SHADOW_HEIGHT = 256;

    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    float near_plane = 1.f, far_plane = 30.f;
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
    glm::mat4 lightView = glm::lookAt(glm::vec3(1, 3, 1),
                                      glm::vec3(0, 0, 0),
                                      glm::vec3(0, 1, 0));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    auto *camera = new Camera(window->getWidthPtr(), window->getHeightPtr(), window->getRatioPtr());
    camera->freeCamera = true;
    while (window->update()) {
        camera->pollEvents(window);
        sniperRifle->rotation.y = (float) glfwGetTime();
        sniperRifle->update();

        if (glfwGetMouseButton(window->getId(), 0) == 1) {
            cube->position.x = camera->position.x + std::cos(camera->rotation.y - 1.57f) * 5.f;
            cube->position.y = camera->position.y + std::sin(-camera->rotation.x) * 5.f;
            cube->position.z = camera->position.z + std::sin(camera->rotation.y - 1.57f) * 5.f;
        }

        glCullFace(GL_FRONT);
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        depthShader->bind();
        depthShader->upload("lightSpaceMatrix", lightSpaceMatrix);
        depthShader->draw(sniperRifle);
        depthShader->draw(map);
        depthShader->draw(cube);
        depthShader->unbind();
        glCullFace(GL_BACK);


        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, window->getWidth(), window->getHeight());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader->bind();
        shader->upload("proj", camera->getPerspective());
        shader->upload("camera.transform", camera->getView());
        shader->upload("camera.position", camera->position);
        shader->upload("environment.sun_position", glm::vec3(-2.0f, 3.0f, -1.0f));
        shader->upload("hasTexture", 1);
        shader->upload("aTexture", 1);
        shader->upload("shadowMap", 0);
        shader->upload("lightSpaceMatrix", lightSpaceMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glActiveTexture(GL_TEXTURE1);
        sniperTexture->bind();
        shader->draw(sniperRifle);
        mapTexture->bind();
        shader->draw(map);
        shader->upload("hasTexture", 0);
        shader->draw(cube);
        shader->unbind();
    }

    window->destroy();
    shader->destroy();
    exit(EXIT_SUCCESS);
}