#include "Shader.h"

Shader::Shader(const std::string& filename) {
    program = glCreateProgram();

    std::string path = "./data/shaders/" + filename + ".vert";
    if(std::filesystem::exists(path)){
        std::ifstream in(path);
        std::string contents((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
        vertex = glCreateShader(GL_VERTEX_SHADER);
        const char* shader_source = contents.c_str();
        glShaderSource(vertex, 1, &shader_source, nullptr);
        glCompileShader(vertex);
        glAttachShader(program, vertex);
        PLOG_DEBUG << "Vertex shader " << filename << " attached";
    }
    path = "./data/shaders/" + filename + ".frag";
    if(std::filesystem::exists(path)){
        std::ifstream in(path);
        std::string contents((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        const char* shader_source = contents.c_str();
        glShaderSource(fragment, 1, &shader_source, nullptr);
        glCompileShader(fragment);
        glAttachShader(program, fragment);
        PLOG_DEBUG << "Fragment shader " << filename << " attached";
    }
    path = "./data/shaders/" + filename + ".geom";
    if(std::filesystem::exists(path)){
        std::ifstream in(path);
        std::string contents((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
        geometry = glCreateShader(GL_GEOMETRY_SHADER);
        const char* shader_source = contents.c_str();
        glShaderSource(geometry, 1, &shader_source, nullptr);
        glCompileShader(geometry);
        glAttachShader(program, geometry);
        PLOG_DEBUG << "Geometry shader " << filename << " attached";
    }

    glLinkProgram(program);
    PLOG_DEBUG << "Shader " << filename << " linked";
}

GLuint Shader::getProgramId() const {
    return program;
}

GLint Shader::getAttribLocation(const char *name) const {
    GLint value = glGetAttribLocation(program, name);
    if(value == -1){
        PLOG_ERROR << "Attrib location is shader not found > " << name;
    }
    return value;
}

GLint Shader::getUniformLocation(const char *name) const {
    GLint value = glGetUniformLocation(program, name);
    if(value == -1){
        PLOG_ERROR << "Uniform location in shader not found > " << name;
    }
    return value;
}

void Shader::bind() const {
    glUseProgram(program);
}

void Shader::unbind() {
    glUseProgram(0);
}
