#ifndef OPENGL_MESH_H
#define OPENGL_MESH_H

#include "glad/gl.h"
#include <vector>
#include "plog/Log.h"

class MyMesh {
    unsigned int VAO, EBO, VBO;
    int indicesCount, stride;
    unsigned long long vertexOffset;
public:
    MyMesh(std::vector<float> *vertices, std::vector<int> *indices, unsigned int vertexSize);

    void draw() const;

    void addParameter(int location, int size);

    void addParameter(int location, int size, bool normalized);

    void destroy();
};


#endif //OPENGL_MESH_H
