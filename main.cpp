#include "Shader.h"
#include <cstdlib>
#include "Window.h"

#include <plog/Log.h>
#include "plog/Initializers/RollingFileInitializer.h"
#include "Model.h"
#include "Camera.h"

int main() {
    std::remove("latest.log");
    plog::init(plog::debug, "latest.log");
    PLOG_INFO << "Logger initialized";
    auto *window = new Window();
    window->setVsync(false);

    auto *shader = new Shader("main");

    auto *sniperRifle = new Model("./data/meshes/sniper.obj");
    sniperRifle->scale *= 2.f;
    auto *monkey = new Model("./data/meshes/monkey.obj");
    monkey->color = {214, 58, 56};
    monkey->color /= 255.f;
    auto *floor = new Model("./data/meshes/floor.obj");
    floor->position.y = -2;
    floor->scale *= 500;
    floor->color *= 0.2f;

    auto *camera = new Camera(window->getWidthPtr(), window->getHeightPtr(), window->getRatioPtr());
    while (window->update()) {
        camera->pollEvents(window);

        sniperRifle->rotation.y = (float) glfwGetTime();
        sniperRifle->position.y = (float) std::sin(glfwGetTime() * 2);

        monkey->position.x = 3;
        monkey->rotation.y = (float) glfwGetTime();

        shader->bind();
        shader->upload("proj", camera->getPerspective());

        shader->upload("camera.transform", camera->getView());
        shader->upload("camera.position", camera->position);

        shader->draw(sniperRifle);
        shader->draw(floor);
        shader->draw(monkey);
        shader->unbind();
    }

    window->destroy();
    shader->destroy();
    exit(EXIT_SUCCESS);
}