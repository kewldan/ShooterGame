#ifndef OPENGL_WINDOW_H
#define OPENGL_WINDOW_H

#include <cstdlib>
#include <iostream>
#include "glad/glad.h"

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <plog/Log.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <chrono>

class Window {
	static bool glfwInitialized;

	static void initializeGlfw();

	GLFWwindow* window;
	bool vsync;
public:
	int width, height;
	float ratio;
	Window();

	Window(int w, int h);

	Window(int w, int h, const char* title);

	~Window();

	void setVsync(bool value);

	[[nodiscard]] bool getVsync() const;

	void setTitle(const char* title);

	void reset() const;

	GLFWwindow* getId();

	bool update(bool* resized);

	void hideCursor();

	void showCursor();

	void setCursorPosition(int x, int y);

	void getCursorPosition(int* x, int* y);

	bool isKeyPressed(int key);

	static void debugDraw(bool value);
};


#endif //OPENGL_WINDOW_H
