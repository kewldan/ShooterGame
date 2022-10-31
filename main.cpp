#include "Shader.h"
#include <cmath>
#include <cstdlib>
#include "Window.h"

#include <plog/Log.h>
#include "plog/Initializers/RollingFileInitializer.h"
#include "Model.h"
#include "Camera.h"
#include <glm/ext.hpp>

unsigned long millis() {
    return std::chrono::system_clock::now().time_since_epoch() /
           std::chrono::milliseconds(1);
}

int main() {
    std::remove("latest.log");
    plog::init(plog::debug, "latest.log");
    PLOG_INFO << "Logger initialized";
    auto *window = new Window();

    window->setVsync(false);

    auto *shader = new Shader("main");
    int projection_location = shader->getUniformLocation("proj");

    auto *sniperRifle = new Model("./data/meshes/sniper.obj", shader);
    sniperRifle->scale *= 0.2f;

    auto *monkey = new Model("./data/meshes/monkey.obj", shader);
    monkey->color = {214, 58, 56};
    monkey->color /= 255.f;

    auto *floor = new Model("./data/meshes/floor.obj", shader);
    floor->position.y = -2;
    floor->scale *= 500;
    floor->color *= 0.2f;

    unsigned long lastFps = millis();
    int fps, fpsCounter;

    auto *camera = new Camera();
    while (window->update()) {
        if (millis() - lastFps > 1000) {
            fps = fpsCounter;
            fpsCounter = 0;
            lastFps = millis();
        }
        if (glfwGetKey(window->getId(), GLFW_KEY_Q) == 1) {
            double x, y;
            glfwGetCursorPos(window->getId(), &x, &y);
            glfwSetCursorPos(window->getId(), (float) window->getWidth() / 2.f, (float) window->getHeight() / 2.f);
            float px = ((float) window->getWidth()) / 2.f - x;
            float py = ((float) window->getHeight()) / 2.f - y;
            camera->rotation.x -= py * 0.001f;
            camera->rotation.y -= px * 0.001f;
            glfwSetInputMode(window->getId(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(window->getId(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        if (glfwGetKey(window->getId(), GLFW_KEY_W) == 1) {
            camera->position.x += std::cos(camera->rotation.y + 1.57f) * 0.03f;
            camera->position.y += std::sin(camera->rotation.x) * 0.03f;
            camera->position.z += std::sin(camera->rotation.y + 1.57f) * 0.03f;
        } else if (glfwGetKey(window->getId(), GLFW_KEY_S) == 1) {
            camera->position.x -= std::cos(camera->rotation.y + 1.57f) * 0.03f;
            camera->position.y -= std::sin(camera->rotation.x) * 0.03f;
            camera->position.z -= std::sin(camera->rotation.y + 1.57f) * 0.03f;
        }

        if (glfwGetKey(window->getId(), GLFW_KEY_A) == 1) {
            camera->position.x += std::cos(camera->rotation.y) * 0.03f;
            camera->position.z += std::sin(camera->rotation.y) * 0.03f;
        } else if (glfwGetKey(window->getId(), GLFW_KEY_D) == 1) {
            camera->position.x -= std::cos(camera->rotation.y) * 0.03f;
            camera->position.z -= std::sin(camera->rotation.y) * 0.03f;
        }

        sniperRifle->rotation.y = glfwGetTime();
        sniperRifle->position.y = std::sin(glfwGetTime() * 2);

        monkey->position.x = 3;
        monkey->rotation.y = glfwGetTime();

        shader->bind();
        glUniformMatrix4fv(projection_location, 1, false, glm::value_ptr(*window->getProj()));
        shader->upload(camera);

        sniperRifle->draw(shader);

        floor->draw(shader);

        monkey->draw(shader);

        window->end();
        glfwSetWindowTitle(window->getId(), (std::string("FPS: ") + std::to_string(fps)).c_str());
        fpsCounter++;
    }
    window->destroy();

    exit(EXIT_SUCCESS);
}