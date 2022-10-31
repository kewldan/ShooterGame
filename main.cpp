#include "Shader.h"
#include <cstdlib>
#include "Window.h"

#include <plog/Log.h>
#include "plog/Initializers/RollingFileInitializer.h"
#include "Model.h"
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

    while (window->update()) {

        model->rotation.y = (float) glfwGetTime();
        model->rotation.z = (float) glfwGetTime();

        shader->bind();
        glUniformMatrix4fv(projection_location, 1, false, glm::value_ptr(*window->getProj()));
        model->draw(shader);

        window->end();
    }
    window->destroy();

    exit(EXIT_SUCCESS);
}