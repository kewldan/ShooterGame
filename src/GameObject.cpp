#include "GameObject.h"

#include "OBJ_Loader.h"

GameObject::GameObject(btDynamicsWorld* world, const char* path, float mass, btCollisionShape* shape, btVector3 position) : collisionShape(shape) {
	meshes = nullptr;
    nbMeshes = 0;
    mvp = new float[16];

    btTransform transform;
    transform.setIdentity();

    if(collisionShape && mass > 0.f){
        collisionShape->calculateLocalInertia(mass, localInertia);
    }

    transform.setOrigin(position);

    motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, localInertia);
    rb = new btRigidBody(rbInfo);

    world->addRigidBody(rb);

    rb->setCollisionShape(collisionShape);

    if(path) loadMeshes(path);
}

void GameObject::draw(Engine::Shader *shader) {
    rb->getWorldTransform().getOpenGLMatrix(mvp);
    shader->uploadMat4("mvp", mvp);
    for (int i = 0; i < nbMeshes; i++) {
        if (meshes[i].hasTexture()) {
            meshes[i].texture->bind();
            shader->upload("hasTexture", 1);
        }
        else {
            shader->upload("hasTexture", 0);
        }
        meshes[i].draw();
    }
}

void GameObject::loadMeshes(const char *path) {
    static char* f = new char[128];
    strcpy_s(f, 128, "./data/meshes/");
    strcat_s(f, 128, path);
    objl::Loader loader;

    if(loader.LoadFile(f)){
        nbMeshes = loader.LoadedMeshes.size();
        meshes = (Mesh*) calloc(nbMeshes, sizeof(Mesh));
        for (size_t i = 0; i < nbMeshes; i++) {
            int dataIndex = 0;
            const auto& curMesh = loader.LoadedMeshes[i];
            meshes[i] = Mesh(curMesh.Vertices.size(), 8, curMesh.Indices.size());
            for (const auto & vertex : curMesh.Vertices)
            {
                meshes[i].data[dataIndex++] = vertex.Position.X;
                meshes[i].data[dataIndex++] = vertex.Position.Y;
                meshes[i].data[dataIndex++] = vertex.Position.Z;

                meshes[i].data[dataIndex++] = vertex.TextureCoordinate.X;
                meshes[i].data[dataIndex++] = 1 - vertex.TextureCoordinate.Y;

                meshes[i].data[dataIndex++] = vertex.Normal.X;
                meshes[i].data[dataIndex++] = vertex.Normal.Y;
                meshes[i].data[dataIndex++] = vertex.Normal.Z;
            }
            memcpy(meshes[i].indices, curMesh.Indices.data(), curMesh.Indices.size() * sizeof(unsigned int));

            if(!curMesh.MeshMaterial.map_Kd.empty()) {
                meshes[i].texture = new Engine::Texture(curMesh.MeshMaterial.map_Kd.c_str());
            }

            meshes[i].upload();

            meshes[i].addParameter(0, 3);
            meshes[i].addParameter(1, 2);
            meshes[i].addParameter(2, 3);
        }
    }
}