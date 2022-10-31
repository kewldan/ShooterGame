#ifndef SHOOTERGAME_CAMERA_H
#define SHOOTERGAME_CAMERA_H

#include <glm/glm.hpp>
#include <glm/ext.hpp>

class Camera {
public:
    glm::vec3 position;
    glm::vec2 rotation;

    Camera();

    glm::mat4 getMatrix();
};


#endif //SHOOTERGAME_CAMERA_H
