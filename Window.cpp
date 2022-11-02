#include "Window.h"

Window::Window() : Window(800, 600) {

}

Window::Window(int w, int h) : Window(w, h, "Untitled") {

}

Window::Window(int w, int h, const char *title) {
    if (!glfwInitialized) {
        initializeGlfw();
    }

    vsync = false;
    width = w;
    height = h;
    ratio = 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, 16);

    window = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (!window) {
        PLOG_FATAL << "Window can not be initialized";
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);

    if (!glfwInitialized) {
        PLOG_INFO << "GLAD loaded";
        gladLoadGL(glfwGetProcAddress);
        glfwInitialized = true;
    }

    glEnable(GL_DEPTH_TEST);
#ifdef NGEBUG
    glEnable(GL_CULL_FACE);
#endif
}

void Window::setVsync(bool value) {
    vsync = value;
    glfwSwapInterval(vsync);
}

bool Window::getVsync() const {
    return vsync;
}

static void error_callback(int error, const char *description) {
    PLOG_ERROR << "[GLFW] " << error << " | " << description;
}

void Window::initializeGlfw() {
    PLOG_INFO << "GLFW initialized";
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        exit(EXIT_FAILURE);
}

GLFWwindow *Window::getId() {
    return window;
}

void Window::destroy() {
    PLOG_DEBUG << "Window destroyed";
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Window::glfwInitialized = false;

bool Window::update() {
    glfwSwapBuffers(window);
    glfwPollEvents();

    glfwGetFramebufferSize(window, &width, &height);
    ratio = (float) width / (float) height;

    glViewport(0, 0, width, height);
    glClearColor(52 / 255.f, 168 / 255.f, 235 / 255.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (millis() - lastFps > 1000) {
        fps = fpsCounter;
        fpsCounter = 0;
        lastFps = millis();
    }

    timeScale = 60.f / (float) fps;
#ifndef NDEBUG
    glfwSetWindowTitle(window, (std::string("FPS: ") + std::to_string(fps)).c_str());
#endif
    fpsCounter++;

    return !glfwWindowShouldClose(window);
}

float Window::getRatio() const {
    return ratio;
}

int Window::getWidth() const {
    return width;
}

int Window::getHeight() const {
    return height;
}

int *Window::getWidthPtr() {
    return &width;
}

int *Window::getHeightPtr() {
    return &height;
}

float *Window::getRatioPtr() {
    return &ratio;
}

unsigned long Window::millis() {
    return std::chrono::system_clock::now().time_since_epoch() /
           std::chrono::milliseconds(1);
}

void Window::hideCursor() {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Window::showCursor() {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}


void Window::setCursorPosition(int x, int y) {
    glfwSetCursorPos(window, x, y);
}

void Window::getCursorPosition(int *x, int *y) {
    double _x, _y;
    glfwGetCursorPos(window, &_x, &_y);
    *x = (int) _x;
    *y = (int) _y;
}

bool Window::isKeyPressed(int key) {
    return glfwGetKey(window, key) == 1;
}

int Window::getFps() const {
    return fps;
}

float Window::getTimeScale() const {
    return timeScale;
}
