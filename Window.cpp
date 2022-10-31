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

    proj = glm::mat4(1);
    ortho = glm::mat4(1);

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
    PLOG_INFO << "GLFW initialized";
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

    proj = glm::perspective(glm::radians(90.f), ratio, 0.01f, 100.f);
    ortho = glm::ortho(0, width, height, 0);

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return !glfwWindowShouldClose(window);
}

float Window::getRatio() const {
    return ratio;
}

glm::mat4 *Window::getOrtho() {
    return &ortho;
}

glm::mat4 *Window::getProj() {
    return &proj;
}
