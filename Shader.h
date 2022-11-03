#ifndef OPENGL_SHADER_H
#define OPENGL_SHADER_H

#include "glad/gl.h"
#include "Camera.h"
#include "Model.h"
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sys/stat.h>
#include <plog/Log.h>

class Shader {
    unsigned int vertex, fragment, geometry, program;
    std::map<std::string, int> *uniforms;
    int8_t shaderParts;
    std::string filename;

    int getUniformLocation(const char *name) const;

    int getAttribLocation(const char *name) const;

public:
    Shader(const std::string &filename);

    unsigned int getProgramId() const;

    void bind();

    void unbind();

    void upload(const char *name, int value) const;

    void upload(const char *name, float value) const;

    void upload(const char *name, glm::vec2 value) const;

    void upload(const char *name, float x, float y) const;

    void upload(const char *name, glm::vec3 value) const;

    void upload(const char *name, float x, float y, float z) const;

    void upload(const char *name, glm::vec4 value) const;

    void upload(const char *name, float x, float y, float z, float w) const;

    void upload(const char *name, glm::mat4 value) const;

    void draw(Model *model) const;

    void destroy() const;
};


#endif //OPENGL_SHADER_H
