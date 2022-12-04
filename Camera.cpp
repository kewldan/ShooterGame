#include "Camera.h"

#include <cmath>

Camera::Camera(Window* window) {
	position = glm::vec3(0);
	rotation = glm::vec2(0);

	this->window = window;
}

glm::mat4& Camera::getView() {
	view = glm::mat4(1);
	view = glm::rotate(view, rotation.x, glm::vec3(1, 0, 0));
	view = glm::rotate(view, rotation.y, glm::vec3(0, 1, 0));
	view = glm::translate(view, -position);

	return view;
}

glm::mat4& Camera::getViewRotationOnly()
{
	view = glm::mat4(1);
	view = glm::rotate(view, rotation.x, glm::vec3(1, 0, 0));
	view = glm::rotate(view, rotation.y, glm::vec3(0, 1, 0));

	return view;
}

const glm::mat4& Camera::getOrthographic() {
	orthographic = glm::ortho(0, window->width, window->height, 0);
	return orthographic;
}

const glm::mat4& Camera::getPerspective() {
	float hfovRad = (float)hFov * 3.1415f / 180;
	float vfovRad = 2.f * std::atan(std::tan(hfovRad / 2) * window->ratio);
	perspective = glm::perspective(vfovRad, window->ratio, 0.1f, 300.f);
	return perspective;
}

constexpr auto PI2 = 6.2831f;

void Camera::pollEvents(RigidBody* player, bool lockCursor) {
	if (lockCursor) {
		int x, y;
		window->getCursorPosition(&x, &y);
		window->setCursorPosition(window->width / 2, window->height / 2);
		int px = window->width / 2 - x;
		int py = window->height / 2 - y;
		rotation.x -= (float)py * 0.001f * sentivity;
		rotation.x = std::max(-1.5f, std::min(rotation.x, 1.5f));
		rotation.y -= (float)px * 0.001f * sentivity;

		//fmod faster version
		if (rotation.y >= PI2) {
			rotation.y -= PI2;
		}
		if (rotation.y <= -PI2) {
			rotation.y += PI2;
		}

		window->hideCursor();
	}
	else {
		window->showCursor();
	}

	float crouch = window->isKeyPressed(GLFW_KEY_LEFT_SHIFT) ? 0.8f : 1.5f;

	Vector3 vel = Vector3::zero();
	if (window->isKeyPressed(GLFW_KEY_W)) {
		vel.x -= std::cos(rotation.y + 1.57f) * 5.f * speed * crouch;
		vel.z -= std::sin(rotation.y + 1.57f) * 5.f * speed * crouch;
	}
	else if (window->isKeyPressed(GLFW_KEY_S)) {
		vel.x += std::cos(rotation.y + 1.57f) * 5.f * speed * crouch;
		vel.z += std::sin(rotation.y + 1.57f) * 5.f * speed * crouch;
	}

	if (window->isKeyPressed(GLFW_KEY_A)) {
		vel.x -= std::cos(rotation.y) * 5.f * speed * crouch;
		vel.z -= std::sin(rotation.y) * 5.f * speed * crouch;
	}
	else if (window->isKeyPressed(GLFW_KEY_D)) {
		vel.x += std::cos(rotation.y) * 5.f * speed * crouch;
		vel.z += std::sin(rotation.y) * 5.f * speed * crouch;
	}
	Vector3 current = player->getLinearVelocity(); //Get curreny velocity (To save Y Axis velocity)
	current.x = vel.x;
	current.z = vel.z;
	player->setLinearVelocity(current);

	Vector3 pos = player->getTransform().getPosition();
	position.x = pos.x;
	position.y = pos.y + 1.5f;
	position.z = pos.z;
}
