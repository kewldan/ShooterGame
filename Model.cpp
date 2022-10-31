#include "Model.h"
#include "OBJ_Loader.h"


Model::Model(std::string filename, Shader *shader) {
    std::vector<unsigned int> indices;
    std::vector<float> output;

    objl::Loader Loader;
    Loader.LoadFile(filename);
    objl::Mesh curMesh = Loader.LoadedMeshes[0];
    for (auto &vertex: curMesh.Vertices) {
        output.push_back(vertex.Position.X);
        output.push_back(vertex.Position.Y);
        output.push_back(vertex.Position.Z);

        output.push_back(vertex.TextureCoordinate.X);
        output.push_back(vertex.TextureCoordinate.Y);

        output.push_back(vertex.Normal.X);
        output.push_back(vertex.Normal.Y);
        output.push_back(vertex.Normal.Z);
    }

    for (int j = 0; j < curMesh.Indices.size(); j += 3) {
        indices.push_back(curMesh.Indices[j]);
        indices.push_back(curMesh.Indices[j + 1]);
        indices.push_back(curMesh.Indices[j + 2]);
    }

    mesh = new Mesh(&output, &indices, 8);
    mesh->addParameter(shader->getAttribLocation("vPos"), 3);
    mesh->addParameter(shader->getAttribLocation("vTexCoord"), 2);
    mesh->addParameter(shader->getAttribLocation("vNormal"), 3);

    scale = glm::vec3(1);
    position = glm::vec3(0);
    rotation = glm::vec3(0);
    color = glm::vec3(1);

    mvp_location = shader->getUniformLocation("mvp");
    color_location = shader->getUniformLocation("material.color");
}

void Model::draw(Shader *shader) {
    mvp = glm::mat4(1.0f);
    mvp = glm::translate(mvp, position);
    mvp = glm::rotate(mvp, rotation.x, glm::vec3(1, 0, 0));
    mvp = glm::rotate(mvp, rotation.y, glm::vec3(0, 1, 0));
    mvp = glm::rotate(mvp, rotation.z, glm::vec3(0, 0, 1));
    mvp = glm::scale(mvp, scale);

    glUniformMatrix4fv(mvp_location, 1, false, glm::value_ptr(mvp));
    glUniform3fv(color_location, 1, glm::value_ptr(color));

    mesh->draw();
}
