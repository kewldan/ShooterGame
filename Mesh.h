#ifndef OPENGL_MESH_H
#define OPENGL_MESH_H

#include "glad/gl.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <vector>
#include "plog/Log.h"

class Mesh {
    unsigned int VAO, EBO, VBO;
    int indicesCount, stride;
    unsigned long long vertexOffset;
public:
    Mesh(std::vector<float> *vertices, std::vector<unsigned int> *indices, unsigned int vertexSize);
    void draw() const;
    void addParameter(int location, int size);
    void addParameter(int location, int size, bool normalized);
    void destroy();
};


#endif //OPENGL_MESH_H
