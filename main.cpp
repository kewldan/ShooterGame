#include "Shader.h"
#include <cmath>
#include <cstdlib>
#include "Window.h"

#include <plog/Log.h>
#include "plog/Initializers/RollingFileInitializer.h"
#include "Model.h"
#include "Camera.h"
#include <glm/ext.hpp>

int main() {
    std::remove("latest.log");
    plog::init(plog::debug, "latest.log");
    PLOG_INFO << "Logger initialized";
    auto *window = new Window();

    auto *shader = new Shader("main");
    int projection_location = shader->getUniformLocation("proj");

    auto *model = new Model("./data/meshes/monkey.obj", shader);
    model->position.z = -3;
    model->color = {168, 50, 50};
    model->color /= 255.f;

    auto *floor = new Model("./data/meshes/floor.obj", shader);
    floor->position.y = -2;
    floor->scale *= 500;
    floor->color *= 0.1f;

    auto *camera = new Camera();
    while (window->update()) {
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
            camera->position.z += std::sin(camera->rotation.y + 1.57f) * 0.03f;
        } else if (glfwGetKey(window->getId(), GLFW_KEY_S) == 1) {
            camera->position.x -= std::cos(camera->rotation.y + 1.57f) * 0.03f;
            camera->position.z -= std::sin(camera->rotation.y + 1.57f) * 0.03f;
        }

        if (glfwGetKey(window->getId(), GLFW_KEY_A) == 1) {
            camera->position.x += std::cos(camera->rotation.y) * 0.03f;
            camera->position.z += std::sin(camera->rotation.y) * 0.03f;
        } else if (glfwGetKey(window->getId(), GLFW_KEY_D) == 1) {
            camera->position.x -= std::cos(camera->rotation.y) * 0.03f;
            camera->position.z -= std::sin(camera->rotation.y) * 0.03f;
        }

        shader->bind();
        glUniformMatrix4fv(projection_location, 1, false, glm::value_ptr(*window->getProj()));
        shader->upload(camera);

        model->draw(shader);

        floor->draw(shader);

        window->end();
    }
    window->destroy();

    exit(EXIT_SUCCESS);
}