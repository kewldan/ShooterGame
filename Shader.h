#ifndef OPENGL_SHADER_H
#define OPENGL_SHADER_H

#include "glad/gl.h"
#include "Camera.h"
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <plog/Log.h>

class Shader {
    unsigned int vertex, fragment, geometry, program;
    int camera_location, camera_pos_location;
    int8_t shaderParts;
    std::string filename;
    bool usingNow;
public:
    Shader(const std::string &filename);

    unsigned int getProgramId() const;

    int getAttribLocation(const char *name) const;

    int getUniformLocation(const char *name) const;

    void bind();

    void unbind();

    void upload(Camera *camera);

    void destroy() const;
};


#endif //OPENGL_SHADER_H
