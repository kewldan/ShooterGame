#include "linmath/include/linmath.h"
#include "Shader.h"
#include <cstdlib>
#include "Window.h"

#include <plog/Log.h>
#include "plog/Initializers/RollingFileInitializer.h"
#include "Mesh.h"
#include <vector>

/*static struct
{
    float x, y;
    float r, g, b;
} vertices[3] =
        {
                { -0.6f, -0.4f, 1.f, 0.f, 0.f},
                {  0.6f, -0.4f, 0.f, 1.f, 0.f },
                {   0.f,  0.6f, 0.f, 0.f, 1.f }
        };*/
/*static float vertices[] = {
        -0.6f, -0.4f, 1.f, 0.f, 0.f,
        0.6f, -0.4f, 0.f, 1.f, 0.f,
        0.f, 0.6f, 0.f, 0.f, 1.f
};*/


int main() {
    std::vector<float> vertices{
            -0.6f, -0.4f, 1.f, 0.f, 0.f,
            0.6f, -0.4f, 0.f, 1.f, 0.f,
            0.f, 0.6f, 0.f, 0.f, 1.f
    };
    std::vector<unsigned int> indices{0, 1, 2};
    plog::init(plog::debug, "latest.log");

    PLOG_DEBUG << "Logger initialized";

    auto *window = new Window();

    auto *shader = new Shader("main");
    GLint projection_location = shader->getUniformLocation("proj");
    GLint model_view_location = shader->getUniformLocation("model");
    GLint vpos_location = shader->getAttribLocation("vPos");
    GLint vcol_location = shader->getAttribLocation("vCol");
    /*unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    PLOG_DEBUG << sizeof(indices) << " ISO";

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    PLOG_DEBUG << sizeof(vertices) << " VSO";

    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
                          sizeof(vertices[0]), (void*) nullptr);
    PLOG_DEBUG << sizeof(vertices[0]) << " VSO0";
    glEnableVertexAttribArray(vcol_location);
    glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE,
                          sizeof(vertices[0]), (void*) (sizeof(float) * 2));*/
    Mesh *mesh = new Mesh(&vertices, &indices, 5);
    mesh->addParameter(vpos_location, 2);
    mesh->addParameter(vcol_location, 3);

    while (window->update()) {
        mat4x4 m, p;

        mat4x4_identity(m);
        mat4x4_rotate_Z(m, m, (float) glfwGetTime());
        mat4x4_scale_aniso(m, m, 2, 2, 2);

        mat4x4_ortho(p, -window->getRatio(), window->getRatio(), -1.f, 1.f, 1.f, -1.f);

        shader->bind();
        glUniformMatrix4fv(projection_location, 1, GL_FALSE, (const GLfloat *) p);
        glUniformMatrix4fv(model_view_location, 1, GL_FALSE, (const GLfloat *) m);

        /*glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(*indices), GL_UNSIGNED_INT, nullptr);*/
        mesh->draw();

        window->end();
    }
    window->destroy();

    exit(EXIT_SUCCESS);
}