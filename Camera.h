#ifndef SHOOTERGAME_CAMERA_H
#define SHOOTERGAME_CAMERA_H

#include <reactphysics3d/reactphysics3d.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Window.h"

using namespace reactphysics3d;

class Camera {
    glm::mat4 orthographic, perspective, view;
    int *width;
    int *height;
    float *ratio;
    int hFov = 70;
public:
    glm::vec3 position;
    glm::vec2 rotation;
    float speed = 1;

    Camera(int *widthPtr, int *heightPtr, float *ratioPtr);

    void pollEvents(Window *window, RigidBody *player);

    glm::mat4 &getView();

    const glm::mat4 &getOrthographic();

    const glm::mat4 &getPerspective();
};


#endif //SHOOTERGAME_CAMERA_H
