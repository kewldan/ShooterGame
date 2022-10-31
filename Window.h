#ifndef OPENGL_WINDOW_H
#define OPENGL_WINDOW_H

#include <cstdlib>
#include <iostream>
#include "glad/gl.h"

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <plog/Log.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

class Window {
    static bool glfwInitialized;

    static void initializeGlfw();

    GLFWwindow *window;
    bool vsync;
    int width, height;
    float ratio;
    glm::mat4 ortho, proj;
public:
    Window();

    Window(int w, int h);

    Window(int w, int h, const char *title);

    void setVsync(bool value);

    [[nodiscard]] bool getVsync() const;

    GLFWwindow *getId();

    bool update();

    [[nodiscard]] float getRatio() const;

    glm::mat4 *getOrtho();

    glm::mat4 *getProj();

    void end();

    void destroy();

    int getWidth() const;

    int getHeight() const;
};


#endif //OPENGL_WINDOW_H
