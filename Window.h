#ifndef OPENGL_WINDOW_H
#define OPENGL_WINDOW_H

#include <cstdlib>
#include <iostream>
#include "glad/gl.h"

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <plog/Log.h>
#include "linmath.h"

class Window {
    static bool glfwInitialized;

    static void initializeGlfw();

    GLFWwindow *window;
    bool vsync;
    int width, height;
    float ratio;
    mat4x4 ortho, proj;
public:
    Window();

    Window(int w, int h);

    Window(int w, int h, const char *title);

    void setVsync(bool value);

    [[nodiscard]] bool getVsync() const;

    GLFWwindow *getId();

    bool update();

    [[nodiscard]] float getRatio() const;

    mat4x4 *getOrtho();

    mat4x4 *getProj();

    void end();

    void destroy();
};


#endif //OPENGL_WINDOW_H
