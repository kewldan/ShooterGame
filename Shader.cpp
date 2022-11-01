#include "Shader.h"

#define SHADER_PART_VERTEX 1
#define SHADER_PART_FRAGMENT 2
#define SHADER_PART_GEOMETRY 4

Shader::Shader(const std::string &filename) {
    usingNow = false;
    this->filename = filename;
    shaderParts = 0;
    program = glCreateProgram();

    uniforms = new std::map<std::string, int>;

    std::string path = "./data/shaders/" + filename + ".vert";
    if (std::filesystem::exists(path)) {
        std::ifstream in(path);
        std::string contents((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
        vertex = glCreateShader(GL_VERTEX_SHADER);
        const char *shader_source = contents.c_str();
        glShaderSource(vertex, 1, &shader_source, nullptr);
        glCompileShader(vertex);

        int length = 0;
        glGetShaderiv(vertex, GL_INFO_LOG_LENGTH, &length);
        if (length > 0) {
            char log[length];
            glGetShaderInfoLog(vertex, length, nullptr, log);
            PLOG_WARNING << "Vertex shader log:\n" << std::string(log);
        }

        int success = 0;
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if (success == GL_TRUE) {
            glAttachShader(program, vertex);
            shaderParts += SHADER_PART_VERTEX;
        } else {
            PLOG_WARNING << "Vertex shader found, but not attached";
        }
    }
    path = "./data/shaders/" + filename + ".frag";
    if (std::filesystem::exists(path)) {
        std::ifstream in(path);
        std::string contents((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        const char *shader_source = contents.c_str();
        glShaderSource(fragment, 1, &shader_source, nullptr);
        glCompileShader(fragment);

        int length = 0;
        glGetShaderiv(fragment, GL_INFO_LOG_LENGTH, &length);
        if (length > 0) {
            char log[length];
            glGetShaderInfoLog(fragment, length, nullptr, log);
            PLOG_WARNING << "Fragment shader log:\n" << std::string(log);
        }

        int success = 0;
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if (success == GL_TRUE) {
            glAttachShader(program, fragment);
            shaderParts += SHADER_PART_FRAGMENT;
        } else {
            PLOG_WARNING << "Fragment shader found, but not attached";
        }
    }
    path = "./data/shaders/" + filename + ".geom";
    if (std::filesystem::exists(path)) {
        std::ifstream in(path);
        std::string contents((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
        geometry = glCreateShader(GL_GEOMETRY_SHADER);
        const char *shader_source = contents.c_str();
        glShaderSource(geometry, 1, &shader_source, nullptr);
        glCompileShader(geometry);

        int length = 0;
        glGetShaderiv(geometry, GL_INFO_LOG_LENGTH, &length);
        if (length > 0) {
            char log[length];
            glGetShaderInfoLog(geometry, length, nullptr, log);
            PLOG_WARNING << "Geometry shader log:\n" << std::string(log);
        }

        int success = 0;
        glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
        if (success == GL_TRUE) {
            glAttachShader(program, geometry);
            shaderParts += SHADER_PART_GEOMETRY;
        } else {
            PLOG_WARNING << "Geometry shader found, but not attached";
        }
    }

    glLinkProgram(program);
    PLOG_INFO << filename << " linked ["
              << ((shaderParts & SHADER_PART_VERTEX) != 0 ? 'V' : '\0')
              << ((shaderParts & SHADER_PART_FRAGMENT) != 0 ? 'F' : '\0')
              << ((shaderParts & SHADER_PART_GEOMETRY) != 0 ? 'G' : '\0')
              << ']';
}

GLuint Shader::getProgramId() const {
    return program;
}

GLint Shader::getAttribLocation(const char *name) const {
    GLint value = glGetAttribLocation(program, name);
    if (value == -1) {
        PLOG_ERROR << "Attrib location in shader not found > " << name;
    }
    return value;
}

GLint Shader::getUniformLocation(const char *name) const {
    if (uniforms->contains(name)) {
        return uniforms->at(name);
    }
    GLint value = glGetUniformLocation(program, name);
    if (value == -1) {
        PLOG_ERROR << "Uniform location in shader not found > " << name;
    }
    (*uniforms)[name] = value;
    return value;
}

void Shader::bind() {
    if (!usingNow) {
        glUseProgram(program);
    }
    usingNow = true;
}

void Shader::unbind() {
    if (usingNow) {
        glUseProgram(0);
    }
    usingNow = false;
}

void Shader::destroy() const {
    if ((shaderParts & SHADER_PART_VERTEX) != 0) {
        glDetachShader(program, vertex);
    }
    if ((shaderParts & SHADER_PART_FRAGMENT) != 0) {
        glDetachShader(program, fragment);
    }
    if ((shaderParts & SHADER_PART_GEOMETRY) != 0) {
        glDetachShader(program, geometry);
    }
    glDeleteProgram(program);
}

void Shader::upload(const char *name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::upload(const char *name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::upload(const char *name, glm::vec2 value) const {
    glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::upload(const char *name, glm::vec3 value) const {
    glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::upload(const char *name, glm::vec4 value) const {
    glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::upload(const char *name, glm::mat4 value) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, false, glm::value_ptr(value));
}

void Shader::draw(Model *model) const {
    upload("mvp", model->getMVP());
    upload("material.color", model->color);
    model->getMesh()->draw();
}
