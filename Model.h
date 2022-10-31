#ifndef OPENGL_MODEL_H
#define OPENGL_MODEL_H

#include "Mesh.h"
#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>

class Model {
    Mesh *mesh;
    int uniform_location;
    glm::mat4 mvp;
public:
    glm::vec3 position, rotation, scale;

    Model(std::string filename, Shader *shader);

    void draw(Shader *shader);
};


#endif //OPENGL_MODEL_H
