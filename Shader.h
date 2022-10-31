#ifndef OPENGL_SHADER_H
#define OPENGL_SHADER_H

#include "glad/gl.h"
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <plog/Log.h>
#include "sha1.h"

class Shader {
    GLuint vertex, fragment, geometry, program;
    int8_t shaderParts;
    std::string filename;
    bool usingNow;
public:
    Shader(const std::string &filename);

    GLuint getProgramId() const;

    GLint getAttribLocation(const char *name) const;

    GLint getUniformLocation(const char *name) const;

    void bind();

    void unbind();

    void destroy() const;
};


#endif //OPENGL_SHADER_H
