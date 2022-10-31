//
// Created by kewld on 31.10.2022.
//

#include "Camera.h"

Camera::Camera() {
    position = glm::vec3(0);
    rotation = glm::vec2(0);
}

glm::mat4 Camera::getMatrix() {
    glm::mat4 mat(1);
    mat = glm::rotate(mat, rotation.x, glm::vec3(1, 0, 0));
    mat = glm::rotate(mat, rotation.y, glm::vec3(0, 1, 0));
    mat = glm::translate(mat, position);

    return mat;
}
