#ifndef OPENGL_SHADER_H
#define OPENGL_SHADER_H

#include "glad/gl.h"
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <plog/Log.h>

class Shader {
    GLuint vertex, fragment, geometry, program;
public:
    Shader(const std::string& filename);
    GLuint getProgramId() const;
    GLint getAttribLocation(const char* name) const;
    GLint getUniformLocation(const char* name) const;
    void bind() const;
    static void unbind();
};


#endif //OPENGL_SHADER_H
