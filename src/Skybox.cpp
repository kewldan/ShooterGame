#include "Skybox.h"

#include "io/Filesystem.h"
#include "stb_image.h"
#include <cstdio>
#include <numeric>

Skybox::Skybox(const char *filename) : mesh(36, 3, 36) {
    glGenTextures(1, &texture);
    bind();

    int width, height, nrChannels;
    char path[256];
    for (unsigned int i = 0; i < 6; i++) {
        std::snprintf(path, sizeof(path), "data/textures/%s%u.jpg", filename, i);
        // Force 3 channels so the GL_RGB upload below is always right.
#ifndef NDEBUG
        unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 3);
#else
        // Release builds read the data embedded as RCDATA resources (see Engine::Texture).
        int size = 0;
        auto *raw = reinterpret_cast<unsigned char *>(Engine::Filesystem::readResourceFile(path, &size));
        unsigned char *data = raw ? stbi_load_from_memory(raw, size, &width, &height, &nrChannels, 3) : nullptr;
#endif
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
        } else {
            PLOGE << "Cube map tex failed to load at path: " << path;
        }
        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    mesh.data = {
            -1.0f, 1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,

            -1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, 1.0f
    };
    std::iota(mesh.indices.begin(), mesh.indices.end(), 0u); // 0, 1, 2, ... 35

    mesh.upload();
    mesh.addParameter(0, 3);
}

Skybox::~Skybox() {
    glDeleteTextures(1, &texture);
}

void Skybox::draw(Engine::Shader *shader, Engine::Camera3D *camera) {
    glDepthFunc(GL_LEQUAL);
    shader->bind();
    shader->upload("proj", camera->getProjection());
    shader->upload("view", camera->getViewRotation());
    shader->upload("skybox", 0);
    glActiveTexture(GL_TEXTURE0);
    bind();

    mesh.draw();
    glDepthFunc(GL_LESS);
}

void Skybox::bind() const {
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
}
