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

unsigned int quadVAO = 0;
unsigned int quadVBO;

void renderQuad() {
    if (quadVAO == 0) {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

int main() {
    std::remove("latest.log");
    plog::init(plog::debug, "latest.log");
    PLOG_INFO << "Logger initialized";
    auto *window = new Window();

    window->setVsync(false);

    auto *shader = new Shader("main");
    auto *simpleDepthShader = new Shader("depth");
    auto *debugQuadShader = new Shader("debug_quad");
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

    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    int lightMatrix_location = simpleDepthShader->getUniformLocation("lightSpaceMatrix");

    glm::vec3 lightPos(-2.0f, 4.0f, -1.0f);

    auto *camera = new Camera();
    while (window->update()) {
        if (millis() - lastFps > 1000) {
            fps = fpsCounter;
            fpsCounter = 0;
            lastFps = millis();
        }
        glfwSetWindowTitle(window->getId(), (std::string("FPS: ") + std::to_string(fps)).c_str());
        fpsCounter++;

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

        sniperRifle->rotation.y = (float) glfwGetTime();
        sniperRifle->position.y = (float) std::sin(glfwGetTime() * 2);

        monkey->position.x = 3;
        monkey->rotation.y = (float) glfwGetTime();

        lightPos.x = sin(glfwGetTime()) * 3.0f;
        lightPos.z = cos(glfwGetTime()) * 2.0f;
        lightPos.y = 5.0 + cos(glfwGetTime()) * 1.0f;


        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glm::mat4 lightProjection, lightView;
        glm::mat4 lightSpaceMatrix;
        float near_plane = 1.0f, far_plane = 7.5f;
        lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
        lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        lightSpaceMatrix = lightProjection * lightView;
        simpleDepthShader->bind();
        glUniformMatrix4fv(lightMatrix_location, 1, GL_FALSE,
                           glm::value_ptr(lightSpaceMatrix));
        sniperRifle->draw(shader);
        floor->draw(shader);
        monkey->draw(shader);
        simpleDepthShader->unbind();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, window->getWidth(), window->getHeight());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        debugQuadShader->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glUniform1i(debugQuadShader->getUniformLocation("depthMap"), 0);
        renderQuad();
        debugQuadShader->unbind();



        /*shader->bind();
        glUniformMatrix4fv(projection_location, 1, false, glm::value_ptr(*window->getProj()));
        shader->upload(camera);
        sniperRifle->draw(shader);
        floor->draw(shader);
        monkey->draw(shader);
        shader->unbind();*/
    }
    window->destroy();

    exit(EXIT_SUCCESS);
}