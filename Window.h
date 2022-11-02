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
#include <chrono>

class Window {
    static bool glfwInitialized;

    static void initializeGlfw();

    GLFWwindow *window;
    bool vsync;
    int width, height;
    float ratio;

    unsigned long lastFps;
    int fps;
    float timeScale;

    static unsigned long millis();

public:
    Window();

    Window(int w, int h);

    Window(int w, int h, const char *title);

    void setVsync(bool value);

    [[nodiscard]] bool getVsync() const;

    GLFWwindow *getId();

    bool update();

    void hideCursor();

    void showCursor();

    void setCursorPosition(int x, int y);

    void getCursorPosition(int *x, int *y);

    bool isKeyPressed(int key);

    static void debugDraw(bool value);

    float getRatio() const;

    void destroy();

    int getWidth() const;

    int getHeight() const;

    int *getWidthPtr();

    int *getHeightPtr();

    float *getRatioPtr();

    int getFps() const;

    float getTimeScale() const;
};


#endif //OPENGL_WINDOW_H
