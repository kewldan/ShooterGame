#include "Camera.h"

#include <cmath>

Camera::Camera(int* widthPtr, int* heightPtr, float* ratioPtr) {
	position = glm::vec3(0);
	rotation = glm::vec2(0);

	width = widthPtr;
	height = heightPtr;
	ratio = ratioPtr;
}

glm::mat4& Camera::getView() {
	view = glm::mat4(1);
	view = glm::rotate(view, rotation.x, glm::vec3(1, 0, 0));
	view = glm::rotate(view, rotation.y, glm::vec3(0, 1, 0));
	view = glm::translate(view, -position);

	return view;
}

const glm::mat4& Camera::getOrthographic() {
	orthographic = glm::ortho(0, *width, *height, 0);
	return orthographic;
}

const glm::mat4& Camera::getPerspective() {
	float hfovRad = (float)hFov * 3.1415f / 180;
	float vfovRad = 2.f * std::atan(std::tan(hfovRad / 2) * *ratio);
	perspective = glm::perspective(vfovRad, *ratio, 0.005f, 500.f);
	return perspective;
}

void Camera::pollEvents(Window* window) {
	if (window->isKeyPressed(GLFW_KEY_Q)) {
		int x, y;
		window->getCursorPosition(&x, &y);
		window->setCursorPosition(window->width / 2, window->height / 2);
		int px = window->width / 2 - x;
		int py = window->height / 2 - y;
		rotation.x -= (float)py * 0.001f;
		rotation.x = std::max(-1.5f, std::min(rotation.x, 1.5f));
		rotation.y -= (float)px * 0.001f;
		window->hideCursor();
	}
	else {
		window->showCursor();
	}

	float crouch = window->isKeyPressed(GLFW_KEY_LEFT_SHIFT) ? 0.8f : 1.5f;
}
