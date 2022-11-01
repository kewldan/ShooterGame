#ifndef OPENGL_MODEL_H
#define OPENGL_MODEL_H

#include "Mesh.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>

class Model {
    Mesh *mesh;
    glm::mat4 mvp;
public:
    glm::vec3 position, rotation, scale, color;

    Model(std::string filename);

    glm::mat4 getMVP();

    Mesh *getMesh() const;
};


#endif //OPENGL_MODEL_H
