#include "Mesh.h"

#include "Engine.h"
#include <cassert>
#include <utility>

void Mesh::draw() const {
    ASSERT("Draw non uploaded mesh", isUploaded());
    // The element buffer binding is part of the VAO state, no need to rebind it here.
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, nullptr);
}

void Mesh::drawRange(int first, int count) const {
    ASSERT("Draw non uploaded mesh", isUploaded());
    ASSERT("Index range out of bounds", first >= 0 && count >= 0 && first + count <= indicesCount);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT,
                   reinterpret_cast<const void *>(static_cast<size_t>(first) * sizeof(unsigned int)));
}

void Mesh::addParameter(int location, int size, bool normalized) {
    ASSERT("Add parameter to non uploaded mesh", isUploaded());
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, size, GL_FLOAT, normalized,
                          stride, reinterpret_cast<const void *>(vertexOffset));
    vertexOffset += sizeof(float) * size;
}

bool Mesh::hasTexture() const {
    return texture != nullptr;
}

bool Mesh::isUploaded() const {
    return VAO != 0 && EBO != 0 && VBO != 0;
}

void Mesh::computeBounds() {
    ASSERT("Bounds need the CPU-side vertex data", !data.empty());
    bounds = AABB();
    for (size_t i = 0; i + 2 < data.size(); i += vertexSize) {
        bounds.extend(glm::vec3(data[i], data[i + 1], data[i + 2]));
    }
}

void Mesh::upload() {
    ASSERT("Mesh already uploaded to GPU memory", !isUploaded());
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                 indices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(data.size() * sizeof(float)), data.data(),
                 GL_STATIC_DRAW);

    // The data now lives on the GPU; drop the CPU copies (dust.obj is several MB).
    std::vector<float>().swap(data);
    std::vector<unsigned int>().swap(indices);
}

Mesh::Mesh(unsigned int vertexCount, unsigned int vertexSize, int indicesCount)
        : data(static_cast<size_t>(vertexCount) * vertexSize), indices(static_cast<size_t>(indicesCount)),
          vertexCount(vertexCount), vertexSize(vertexSize), indicesCount(indicesCount),
          stride(static_cast<int>(vertexSize * sizeof(float))) {
    ASSERT("Indices count <= 0", indicesCount > 0);
}

Mesh::~Mesh() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
}

Mesh::Mesh(Mesh &&other) noexcept
        : VAO(other.VAO), EBO(other.EBO), VBO(other.VBO), vertexOffset(other.vertexOffset),
          data(std::move(other.data)), indices(std::move(other.indices)), texture(std::move(other.texture)),
          bounds(other.bounds), vertexCount(other.vertexCount), vertexSize(other.vertexSize), indicesCount(other.indicesCount),
          stride(other.stride) {
    other.VAO = other.EBO = other.VBO = 0;
}

Mesh &Mesh::operator=(Mesh &&other) noexcept {
    if (this != &other) {
        std::swap(VAO, other.VAO);
        std::swap(EBO, other.EBO);
        std::swap(VBO, other.VBO);
        std::swap(vertexOffset, other.vertexOffset);
        data.swap(other.data);
        indices.swap(other.indices);
        texture.swap(other.texture);
        std::swap(bounds, other.bounds);
        std::swap(vertexCount, other.vertexCount);
        std::swap(vertexSize, other.vertexSize);
        std::swap(indicesCount, other.indicesCount);
        std::swap(stride, other.stride);
    }
    return *this;
}
