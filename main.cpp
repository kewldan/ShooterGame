#include "Shader.h"
#include <cstdlib>
#include "Window.h"

#include <plog/Log.h>
#include "plog/Initializers/RollingFileInitializer.h"
#include "Mesh.h"
#include <vector>

std::vector<std::string> split(std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

struct Vector3 {
    float x, y, z;
};

int main() {
    std::ifstream input("./data/meshes/cube.obj");
    std::vector<float> vertices;
    std::vector<Vector3> normals;
    std::vector<unsigned int> indices;

    for (std::string line; getline(input, line);) {
        std::vector<std::string> v = split(line, " ");
        if (v[0] == "v") {
            vertices.push_back(std::stof(v[1]));
            vertices.push_back(std::stof(v[2]));
            vertices.push_back(std::stof(v[3]));
        } else if (v[0] == "vn") {
            normals.push_back(
                    {
                            std::stof(v[1]),
                            std::stof(v[2]),
                            std::stof(v[3])
                    }
            );
        } else if (v[0] == "f") {
            std::vector<std::string> token0 = split(v[1], "//");
            std::vector<std::string> token1 = split(v[2], "//");
            std::vector<std::string> token2 = split(v[3], "//");
            int face_normals[] = {
                    std::stoi(token0[0]) - 1,
                    std::stoi(token1[0]) - 1,
                    std::stoi(token2[0]) - 1,
            };

            indices.push_back(std::stoi(token0[0]) - 1);
            indices.push_back(std::stoi(token1[0]) - 1);
            indices.push_back(std::stoi(token2[0]) - 1);
        }
    }

    plog::init(plog::debug, "latest.log");

    PLOG_DEBUG << "Logger initialized";

    auto *window = new Window();

    auto *shader = new Shader("main");
    GLint projection_location = shader->getUniformLocation("proj");
    GLint model_view_location = shader->getUniformLocation("model");
    GLint vpos_location = shader->getAttribLocation("vPos");

    Mesh *mesh = new Mesh(&vertices, &indices, 3);
    mesh->addParameter(vpos_location, 3);

    while (window->update()) {
        mat4x4 m;

        mat4x4_identity(m);
        mat4x4_translate(m, 0, 0, -5);
        mat4x4_rotate_Z(m, m, (float) glfwGetTime());
        mat4x4_rotate_Y(m, m, (float) glfwGetTime());

        shader->bind();
        glUniformMatrix4fv(projection_location, 1, GL_FALSE, (const GLfloat *) window->getProj());
        glUniformMatrix4fv(model_view_location, 1, GL_FALSE, (const GLfloat *) m);
        mesh->draw();

        window->end();
    }
    window->destroy();

    exit(EXIT_SUCCESS);
}