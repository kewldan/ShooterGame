#include "Shader.h"

#define SHADER_PART_VERTEX 1
#define SHADER_PART_FRAGMENT 2
#define SHADER_PART_GEOMETRY 4
#define SHADER_PART_COMPUTE 8

Shader::Shader(const std::string &filename) {
    usingNow = false;
    this->filename = filename;
    shaderParts = 0;
    program = glCreateProgram();

#ifndef NDEBUG
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
              << ((shaderParts & SHADER_PART_COMPUTE) != 0 ? 'C' : '\0')
              << ']';

    GLint length = 0;
    glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &length);

    std::vector<GLubyte> buffer(length);
    GLenum format = 0;
    glGetProgramBinary(program, length, nullptr, &format, buffer.data());

    std::string fName("./data/shaders/" + filename + ".bin");
    std::ofstream out(fName.c_str(), std::ios::binary);
    out.write(reinterpret_cast<char *>(buffer.data()), length);
    out.close();

    PLOG_INFO << "Shader cache created [" << format << ']';
#else
    std::string path = "./data/shaders/" + filename + ".bin";
    if (std::filesystem::exists(path)) {
        std::ifstream infile(path, std::ios_base::binary);
        std::vector<char> buffer((std::istreambuf_iterator<char>(infile)),
                                 std::istreambuf_iterator<char>());
        glProgramBinary(program, 36385, &buffer[0], (int) buffer.size());

        int length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        if (length > 0) {
            char log[length];
            glGetProgramInfoLog(program, length, nullptr, log);
            PLOG_WARNING << "Program " << filename << " log:\n" << std::string(log);
        }

        int success = 0;
        glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
        if (success == GL_FALSE) {
            PLOG_ERROR << "Program " << filename << " not validated!";
        } else {
            PLOG_INFO << filename << " used cache";
            return;
        }
    }
#endif
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
    GLint value = glGetUniformLocation(program, name);
    if (value == -1) {
        PLOG_ERROR << "Uniform location in shader not found > " << name;
    }
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
