#include "GameObject.h"

#include "Engine.h"
#include "OBJ_Loader.h"
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <string>

GameObject::GameObject(btDynamicsWorld *world, const char *path, float mass, btCollisionShape *shape,
                       btVector3 position) : world(world), collisionShape(shape) {
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

    if (path) loadMeshes(path);
}

GameObject::~GameObject() {
    world->removeRigidBody(rb.get());
}

void GameObject::draw(Engine::Shader *shader) {
    rb->getWorldTransform().getOpenGLMatrix(mvp);
    shader->uploadMat4("mvp", mvp);
    for (const auto &mesh: meshes) {
        if (mesh.hasTexture()) {
            mesh.texture->bind();
            shader->upload("hasTexture", 1);
        } else {
            shader->upload("hasTexture", 0);
        }
        mesh.draw();
    }
}

void GameObject::loadMeshes(const char *path) {
    const std::string file = std::string("./data/meshes/") + path;
    objl::Loader loader;

    if (!loader.LoadFile(file)) {
        PLOGE << "Failed to load mesh [" << file << "]";
        return;
    }

    meshes.clear();
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
        std::copy(curMesh.Indices.begin(), curMesh.Indices.end(), mesh.indices.begin());

        if (!curMesh.MeshMaterial.map_Kd.empty()) {
            // Engine::Texture always looks in data/textures/, so keep only the file name
            // (the .mtl files reference textures as "../textures/x.png").
            const std::string textureName = std::filesystem::path(curMesh.MeshMaterial.map_Kd).filename().string();
            mesh.texture = std::make_unique<Engine::Texture>(textureName.c_str());
        }

        mesh.upload();

        mesh.addParameter(0, 3);
        mesh.addParameter(1, 2);
        mesh.addParameter(2, 3);
    }
}

void GameObject::setCastShadows(bool value) {
    this->castShadows = value;
}

bool GameObject::isCastShadows() const {
    return this->castShadows;
}
