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
    window->setVsync(false);

    auto *shader = new Shader("main");

    auto *sniperRifle = new Model("./data/meshes/sniper.obj");
    sniperRifle->scale *= 2.f;
    auto *map = new Model("./data/meshes/map.obj");
    map->scale *= 10.f;

    auto *sniperTexture = new Texture("./data/textures/sniper.png");
    auto *mapTexture = new Texture("./data/textures/palette.png");

    auto *camera = new Camera(window->getWidthPtr(), window->getHeightPtr(), window->getRatioPtr());
    camera->freeCamera = false;
    while (window->update()) {
        camera->pollEvents(window);

        sniperRifle->rotation.y = (float) glfwGetTime();

        shader->bind();

        shader->upload("proj", camera->getPerspective());

        shader->upload("camera.transform", camera->getView());
        shader->upload("camera.position", camera->position);
        shader->upload("hasTexture", 1);
        shader->upload("aTexture", 0);

        sniperRifle->update();

        glActiveTexture(GL_TEXTURE0);
        sniperTexture->bind();
        shader->draw(sniperRifle);

        mapTexture->bind();
        shader->draw(map);

        shader->unbind();

    }

    window->destroy();
    shader->destroy();
    exit(EXIT_SUCCESS);
}