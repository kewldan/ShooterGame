#include "Shader.h"
#include <cstdlib>
#include "Window.h"

#include <plog/Log.h>
#include "plog/Initializers/RollingFileInitializer.h"
#include "Mesh.h"
#include <vector>

std::vector<std::string> split(const std::string &s, const std::string &delimiter) {
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

struct Vector2 {
    float x, y;
};

struct IdxGroup {
    int idxPos, idxVecNormal, idxTexCoord;
};

int main() {
    std::ifstream input("./data/meshes/monkey.obj");

    std::vector<Vector3> vertices;
    std::vector<Vector3> normals;
    std::vector<Vector2> texCoord;
    std::vector<std::vector<IdxGroup>> faces;

    for (std::string line; getline(input, line);) {
        std::vector<std::string> v = split(line, " ");
        if (v[0] == "v") {
            vertices.push_back(
                    {
                            std::stof(v[1]),
                            std::stof(v[2]),
                            std::stof(v[3])
                    }
            );
        } else if (v[0] == "vn") {
            normals.push_back(
                    {
                            std::stof(v[1]),
                            std::stof(v[2]),
                            std::stof(v[3])
                    }
            );
        } else if (v[0] == "vt") {
            texCoord.push_back(
                    {
                            std::stof(v[1]),
                            std::stof(v[2])
                    }
            );
        } else if (v[0] == "f") {
            std::vector<std::string> token0 = split(v[1], "/");
            std::vector<std::string> token1 = split(v[2], "/");
            std::vector<std::string> token2 = split(v[3], "/");

            std::vector<IdxGroup> face{
                    {std::stoi(token0[0]) - 1, !token0[1].empty() ? std::stoi(token0[1]) - 1 : -1,
                                                                                                   std::stoi(
                                                                                                           token0[2]) -
                                                                                                   1},
                    {std::stoi(token1[0]) - 1, !token1[1].empty() ? std::stoi(token1[1]) - 1 : -1, std::stoi(
                            token1[2]) - 1},
                    {std::stoi(token2[0]) - 1, !token2[1].empty() ? std::stoi(token2[1]) - 1 : -1, std::stoi(
                            token2[2]) - 1}
            };
            faces.push_back(face);
        }
    }

    std::vector<unsigned int> indices;
    float vertex_positions[vertices.size() * 3];
    float vertex_texcoord[vertices.size() * 2];
    float vertex_normals[vertices.size() * 3];

    for (int i = 0; i < vertices.size(); i++) {
        Vector3 pos = vertices[i];
        vertex_positions[i * 3] = pos.x;
        vertex_positions[i * 3 + 1] = pos.y;
        vertex_positions[i * 3 + 2] = pos.z;
    }

    for (std::vector<IdxGroup> face: faces) {
        for (int i = 0; i < 3; i++) {
            IdxGroup group = face[i];
            indices.push_back(group.idxPos);

            vertex_texcoord[group.idxPos * 2] = texCoord[group.idxTexCoord].x;
            vertex_texcoord[group.idxPos * 2 + 1] = texCoord[group.idxTexCoord].y;

            vertex_normals[group.idxPos * 3] = normals[group.idxVecNormal].x;
            vertex_normals[group.idxPos * 3 + 1] = normals[group.idxVecNormal].y;
            vertex_normals[group.idxPos * 3 + 2] = normals[group.idxVecNormal].z;
        }
    }

    std::vector<float> output;
    for (int i = 0; i < vertices.size() * 3; i += 3) {
        output.push_back(vertex_positions[i]);
        output.push_back(vertex_positions[i + 1]);
        output.push_back(vertex_positions[i + 2]);

        output.push_back(vertex_texcoord[i]);
        output.push_back(vertex_texcoord[i + 1]);

        output.push_back(vertex_normals[i]);
        output.push_back(vertex_normals[i + 1]);
        output.push_back(vertex_normals[i + 2]);
    }

    plog::init(plog::debug, "latest.log");

    PLOG_DEBUG << "Logger initialized";

    auto *window = new Window();

    auto *shader = new Shader("main");
    GLint projection_location = shader->getUniformLocation("proj");
    GLint model_view_location = shader->getUniformLocation("model");
    GLint vpos_location = shader->getAttribLocation("vPos");
    GLint vtexcoord_location = shader->getAttribLocation("vTexCoord");
    GLint vnorm_location = shader->getAttribLocation("vNorm");

    Mesh *mesh = new Mesh(&output, &indices, 8);
    mesh->addParameter(vpos_location, 3);
    mesh->addParameter(vtexcoord_location, 2);
    mesh->addParameter(vnorm_location, 3);

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