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

    if (!collisionShape) {
        // Bullet dereferences the shape when the body is added to the world, so a null shape would crash;
        // build one from the model instead (static only: a moving triangle mesh is not supported).
        ASSERT("A mesh collider needs a mesh", path != nullptr);
        ASSERT("A mesh collider must be static", mass == 0.f);
        triangleMesh = std::make_unique<btTriangleMesh>();
    }
    if (path) loadMeshes(path, chunkSize);
    if (!collisionShape) {
        if (triangleMesh->getNumTriangles() == 0) {
            PLOGE << "No triangles for the mesh collider of [" << path << "], using a unit box";
            collisionShape = std::make_unique<btBoxShape>(btVector3(0.5f, 0.5f, 0.5f));
        } else {
            collisionShape = std::make_unique<btBvhTriangleMeshShape>(triangleMesh.get(), true);
        }
    }

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(position);

    if (mass > 0.f) {
        collisionShape->calculateLocalInertia(mass, localInertia);
    }

    motionState = std::make_unique<btDefaultMotionState>(transform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState.get(), collisionShape.get(), localInertia);
    rb = std::make_unique<btRigidBody>(rbInfo);
    // Lets a raycast hit be traced back to the object (see Player::fire).
    rb->setUserPointer(this);

    world->addRigidBody(rb.get());
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

bool GameObject::loadObj(const char *path, float chunkSize, std::vector<Mesh> &meshes, std::vector<Chunk> &chunks,
                         btTriangleMesh *triangles) {
    const std::string file = std::string("./data/meshes/") + path;
    objl::Loader loader;

    if (!loader.LoadFile(file)) {
        PLOGE << "Failed to load mesh [" << file << "]";
        return false;
    }

    meshes.clear();
    chunks.clear();
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
            const auto &a = curMesh.Vertices[curMesh.Indices[triangle]].Position;
            const auto &b = curMesh.Vertices[curMesh.Indices[triangle + 1]].Position;
            const auto &c = curMesh.Vertices[curMesh.Indices[triangle + 2]].Position;
            std::pair<int, int> cell{0, 0};
            if (chunkSize > 0.f) {
                cell.first = static_cast<int>(std::floor((a.X + b.X + c.X) / 3.f / chunkSize));
                cell.second = static_cast<int>(std::floor((a.Z + b.Z + c.Z) / 3.f / chunkSize));
            }
            cells[cell].push_back(static_cast<unsigned int>(triangle));
            if (triangles) {
                triangles->addTriangle(btVector3(a.X, a.Y, a.Z), btVector3(b.X, b.Y, b.Z), btVector3(c.X, c.Y, c.Z));
            }
        }

        // Write the indices cell by cell; every cell becomes a chunk with the bounds of its triangles.
        size_t indexOffset = 0;
        for (const auto &[cell, cellTriangles]: cells) {
            Chunk chunk{meshes.size() - 1, static_cast<int>(indexOffset), static_cast<int>(cellTriangles.size() * 3), {}};
            for (const unsigned int triangle: cellTriangles) {
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
    PLOGI << "Loaded [" << file << "]: " << meshes.size() << " meshes, " << chunks.size() << " chunks"
          << (triangles ? ", " + std::to_string(triangles->getNumTriangles()) + " collision triangles" : "");
    return true;
}

void GameObject::loadMeshes(const char *path, float chunkSize) {
    worldBounds.clear();
    loadObj(path, chunkSize, meshes, chunks, triangleMesh.get());
}

void GameObject::setBoxMesh(const glm::vec3 &half, const char *texture) {
    meshes.clear();
    chunks.clear();
    worldBounds.clear();
    // 4 vertices per face (own normal and UVs), 6 faces.
    Mesh &mesh = meshes.emplace_back(24u, 8u, 36);
    const glm::vec3 normals[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    size_t d = 0;
    for (int face = 0; face < 6; face++) {
        const glm::vec3 n = normals[face];
        // Two tangents spanning the face, with a winding that faces outwards.
        const glm::vec3 helper = std::abs(n.y) > 0.5f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
        const glm::vec3 t = glm::normalize(glm::cross(helper, n)), b = glm::cross(n, t);
        const glm::vec2 uvs[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        for (int v = 0; v < 4; v++) {
            const float sx = (v == 1 || v == 2) ? 1.f : -1.f, sy = (v >= 2) ? 1.f : -1.f;
            const glm::vec3 p = (n + t * sx + b * sy) * half;
            mesh.data[d++] = p.x;
            mesh.data[d++] = p.y;
            mesh.data[d++] = p.z;
            mesh.data[d++] = uvs[v].x;
            mesh.data[d++] = uvs[v].y;
            mesh.data[d++] = n.x;
            mesh.data[d++] = n.y;
            mesh.data[d++] = n.z;
        }
        const unsigned int base = face * 4;
        const unsigned int quad[6] = {base, base + 1, base + 2, base, base + 2, base + 3};
        std::copy(std::begin(quad), std::end(quad), mesh.indices.begin() + face * 6);
    }
    if (texture) {
        mesh.texture = std::make_unique<Engine::Texture>(texture);
    }
    mesh.computeBounds();
    chunks.push_back({0, 0, 36, mesh.bounds});
    mesh.upload();
    mesh.addParameter(0, 3);
    mesh.addParameter(1, 2);
    mesh.addParameter(2, 3);
}

void GameObject::setCastShadows(bool value) {
    this->castShadows = value;
}

bool GameObject::isCastShadows() const {
    return this->castShadows;
}
