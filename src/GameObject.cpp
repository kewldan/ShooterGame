#include "GameObject.h"

#include "Engine.h"
#include "OBJ_Loader.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <map>
#include <string>
#include <utility>

GameObject::GameObject(btDynamicsWorld *world, const char *path, float mass, btCollisionShape *shape,
                       btVector3 position, float chunkSize) : world(world), collisionShape(shape) {
    ASSERT("World is nullptr", world != nullptr);
    // Bullet dereferences the shape when the body is added to the world, so a null shape would crash.
    ASSERT("Collision shape is nullptr", shape != nullptr);

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(position);

    if (collisionShape && mass > 0.f) {
        collisionShape->calculateLocalInertia(mass, localInertia);
    }

    motionState = std::make_unique<btDefaultMotionState>(transform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState.get(), collisionShape.get(), localInertia);
    rb = std::make_unique<btRigidBody>(rbInfo);

    world->addRigidBody(rb.get());

    if (path) loadMeshes(path, chunkSize);
}

GameObject::~GameObject() {
    world->removeRigidBody(rb.get());
}

void GameObject::updateWorldBounds() {
    const glm::mat4 model = glm::make_mat4(mvp);
    if (worldBounds.size() == chunks.size() && model == worldBoundsModel) {
        return;
    }
    worldBoundsModel = model;
    worldBounds.resize(chunks.size());
    for (size_t i = 0; i < chunks.size(); i++) {
        worldBounds[i] = chunks[i].bounds.transformed(model);
    }
}

void GameObject::draw(Engine::Shader *shader, const Frustum *frustum, CullStats *stats) {
    rb->getWorldTransform().getOpenGLMatrix(mvp);
    shader->uploadMat4("mvp", mvp);
    if (frustum) {
        updateWorldBounds();
    }
    // Depth-only shaders (shadow pass) have no hasTexture uniform; skip the upload there
    // instead of making the Engine log a "uniform not found" error.
    const bool wantsTexture = shader->hasUniform("hasTexture");

    size_t c = 0;
    for (size_t m = 0; m < meshes.size(); m++) {
        const Mesh &mesh = meshes[m];
        // Texture/uniform state is set lazily, only when a chunk of this mesh is actually drawn.
        bool bound = false;
        int runFirst = 0, runCount = 0;
        const auto flush = [&] {
            if (runCount == 0) return;
            if (!bound) {
                if (mesh.hasTexture()) {
                    mesh.texture->bind();
                }
                if (wantsTexture) {
                    shader->upload("hasTexture", mesh.hasTexture() ? 1 : 0);
                }
                bound = true;
            }
            mesh.drawRange(runFirst, runCount);
            if (stats) stats->drawCalls++;
            runCount = 0;
        };

        for (; c < chunks.size() && chunks[c].mesh == m; c++) {
            const Chunk &chunk = chunks[c];
            if (frustum && !frustum->intersects(worldBounds[c])) {
                if (stats) stats->culled++;
                flush();
                continue;
            }
            if (stats) stats->drawn++;
            // Chunks are stored in index order, so a run of visible chunks is one contiguous range.
            if (runCount == 0) runFirst = chunk.firstIndex;
            runCount += chunk.indexCount;
        }
        flush();
    }
}

void GameObject::loadMeshes(const char *path, float chunkSize) {
    const std::string file = std::string("./data/meshes/") + path;
    objl::Loader loader;

    if (!loader.LoadFile(file)) {
        PLOGE << "Failed to load mesh [" << file << "]";
        return;
    }

    meshes.clear();
    chunks.clear();
    worldBounds.clear();
    meshes.reserve(loader.LoadedMeshes.size());
    for (const auto &curMesh: loader.LoadedMeshes) {
        if (curMesh.Indices.empty()) continue;

        Mesh &mesh = meshes.emplace_back(static_cast<unsigned int>(curMesh.Vertices.size()), 8u,
                                         static_cast<int>(curMesh.Indices.size()));
        size_t dataIndex = 0;
        for (const auto &vertex: curMesh.Vertices) {
            mesh.data[dataIndex++] = vertex.Position.X;
            mesh.data[dataIndex++] = vertex.Position.Y;
            mesh.data[dataIndex++] = vertex.Position.Z;

            mesh.data[dataIndex++] = vertex.TextureCoordinate.X;
            mesh.data[dataIndex++] = 1 - vertex.TextureCoordinate.Y;

            mesh.data[dataIndex++] = vertex.Normal.X;
            mesh.data[dataIndex++] = vertex.Normal.Y;
            mesh.data[dataIndex++] = vertex.Normal.Z;
        }

        // Bucket the triangles by the grid cell their centroid falls in (a single cell without chunking);
        // std::map keeps the cells in (x, z) order so that neighbouring cells are neighbouring index ranges.
        std::map<std::pair<int, int>, std::vector<unsigned int>> cells;
        for (size_t triangle = 0; triangle + 2 < curMesh.Indices.size(); triangle += 3) {
            std::pair<int, int> cell{0, 0};
            if (chunkSize > 0.f) {
                const auto &a = curMesh.Vertices[curMesh.Indices[triangle]].Position;
                const auto &b = curMesh.Vertices[curMesh.Indices[triangle + 1]].Position;
                const auto &c = curMesh.Vertices[curMesh.Indices[triangle + 2]].Position;
                cell.first = static_cast<int>(std::floor((a.X + b.X + c.X) / 3.f / chunkSize));
                cell.second = static_cast<int>(std::floor((a.Z + b.Z + c.Z) / 3.f / chunkSize));
            }
            cells[cell].push_back(static_cast<unsigned int>(triangle));
        }

        // Write the indices cell by cell; every cell becomes a chunk with the bounds of its triangles.
        size_t indexOffset = 0;
        for (const auto &[cell, triangles]: cells) {
            Chunk chunk{meshes.size() - 1, static_cast<int>(indexOffset), static_cast<int>(triangles.size() * 3), {}};
            for (const unsigned int triangle: triangles) {
                for (int k = 0; k < 3; k++) {
                    const unsigned int index = curMesh.Indices[triangle + k];
                    mesh.indices[indexOffset++] = index;
                    const auto &position = curMesh.Vertices[index].Position;
                    chunk.bounds.extend(glm::vec3(position.X, position.Y, position.Z));
                }
            }
            chunks.push_back(chunk);
        }

        if (!curMesh.MeshMaterial.map_Kd.empty()) {
            // Engine::Texture always looks in data/textures/, so keep only the file name
            // (the .mtl files reference textures as "../textures/x.png").
            const std::string textureName = std::filesystem::path(curMesh.MeshMaterial.map_Kd).filename().string();
            mesh.texture = std::make_unique<Engine::Texture>(textureName.c_str());
        }

        mesh.computeBounds();
        mesh.upload();

        mesh.addParameter(0, 3);
        mesh.addParameter(1, 2);
        mesh.addParameter(2, 3);
    }
    PLOGI << "Loaded [" << file << "]: " << meshes.size() << " meshes, " << chunks.size() << " chunks";
}

void GameObject::setCastShadows(bool value) {
    this->castShadows = value;
}

bool GameObject::isCastShadows() const {
    return this->castShadows;
}
