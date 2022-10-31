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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, 16);

    window = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (!window) {
        PLOG_FATAL << "Window cant be initialized";
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);

    if (!glfwInitialized) {
        PLOG_DEBUG << "Glad loaded";
        gladLoadGL(glfwGetProcAddress);
        glfwInitialized = true;
    }

    glEnable(GL_DEPTH_TEST);
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
    PLOG_DEBUG << "GLFW initialized";
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        exit(EXIT_FAILURE);
}

GLFWwindow *Window::getId() {
    return window;
}

void Window::end() {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void Window::destroy() {
    PLOG_DEBUG << "Window destroyed";
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Window::glfwInitialized = false;

bool Window::update() {
    glfwGetFramebufferSize(window, &width, &height);
    ratio = (float) width / (float) height;

    mat4x4_ortho(ortho, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
    mat4x4_perspective(proj, 90 * (3.1415f / 180), ratio, 0.01f, 100.0);

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return !glfwWindowShouldClose(window);
}

float Window::getRatio() const {
    return ratio;
}

mat4x4 *Window::getOrtho() {
    return &ortho;
}

mat4x4 *Window::getProj() {
    return &proj;
}
