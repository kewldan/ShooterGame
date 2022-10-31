#ifndef OPENGL_MODEL_H
#define OPENGL_MODEL_H

#include "Mesh.h"
#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>

class Model {
    Mesh *mesh;
    int mvp_location, color_location;
    glm::mat4 mvp;
public:
    glm::vec3 position, rotation, scale, color;

    Model(std::string filename, Shader *shader);

    void draw(Shader *shader);
};


#endif //OPENGL_MODEL_H
