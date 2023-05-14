#include "Model.h"
#include "OBJ_Loader.h"

Model::Model(const char* filename, PhysicsWorld* world, PhysicsCommon* common, bool createConcaveCollider) {
	int nb = -2;
	char* f = new char[128];
	strcpy_s(f, 128,filename);

	MeshData* meshesData = loadMesh(f, &nb);
	new (this) Model(meshesData, nb, world, common, createConcaveCollider);
}

Model::Model(MeshData* data, int nb, PhysicsWorld* world, PhysicsCommon* common, bool createConcaveCollider)
{
	meshes = (MyMesh*)calloc(nb, sizeof(MyMesh));
	for (int i = 0; i < nb; i++) {
		meshes[i] = MyMesh(data[i].output, data[i].indices, 8);
		meshes[i].addParameter(0, 3);
		meshes[i].addParameter(1, 2);
		meshes[i].addParameter(2, 3);
		if (strlen(data[i].texturePath) > 0) {
			int nameIndex = 0;
			for (int j = (int) strlen(data[i].texturePath) - 1; j > 0; j--) {
				if (data[i].texturePath[j] == '/' || data[i].texturePath[j] == '\\') {
					nameIndex = j;
					break;
				}
			}
			meshes[i].texture = new Engine::Texture(data[i].texturePath + nameIndex + 1);
		}
	}
	nbMeshes = nb;

	rb = world->createRigidBody(Transform::identity());

	mvp = new float[16];

	if (createConcaveCollider) {
		rb->setType(BodyType::STATIC);
		TriangleMesh* triangleMesh = common->createTriangleMesh();
		for (int i = 0; i < nb; i++) {
			auto* triangleArray =
				new TriangleVertexArray(
					data[i].vertices->size() / 3, data[i].vertices->data(), 3 * sizeof(float),
					data[i].normals->data(), 3 * sizeof(float),
					data[i].indices->size() / 3, data[i].indices->data(), 3 * sizeof(int),
					TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE,
					TriangleVertexArray::NormalDataType::NORMAL_FLOAT_TYPE,
					TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE);
			triangleMesh->addSubpart(triangleArray);
		}
		rb->addCollider(common->createConcaveMeshShape(triangleMesh), Transform::identity());
	}
}

MeshData* Model::loadMesh(const char* filename, int* len)
{
	static char* f = new char[128];
	strcpy_s(f, 128, "./data/meshes/");
	strcat_s(f, 128, filename);
    objl::Loader loader;

    if(loader.LoadFile(f)){
        *len = loader.LoadedMeshes.size();
        auto* data = new MeshData[*len];
        for (int i = 0; i < *len; i++) {
            const auto& curMesh = loader.LoadedMeshes[i];
            for (const auto & vertex : curMesh.Vertices)
            {
                data[i].vertices->push_back(vertex.Position.X);
                data[i].vertices->push_back(vertex.Position.Y);
                data[i].vertices->push_back(vertex.Position.Z);

                data[i].normals->push_back(vertex.Normal.X);
                data[i].normals->push_back(vertex.Normal.Y);
                data[i].normals->push_back(vertex.Normal.Z);

                data[i].output->push_back(vertex.Position.X);
                data[i].output->push_back(vertex.Position.Y);
                data[i].output->push_back(vertex.Position.Z);

                data[i].output->push_back(vertex.TextureCoordinate.X);
                data[i].output->push_back(1 - vertex.TextureCoordinate.Y);

                data[i].output->push_back(vertex.Normal.X);
                data[i].output->push_back(vertex.Normal.Y);
                data[i].output->push_back(vertex.Normal.Z);
            }

            for (const auto& index : curMesh.Indices)
            {
                data[i].indices->push_back(index);
            }

            strcpy_s(data[i].texturePath, 128, curMesh.MeshMaterial.map_Kd.c_str());
        }
        PLOGI << "Model [" << filename << "] loaded " << *len << " meshes";
        return data;
    }else{
        return nullptr;
    }
}

float* Model::getMVP() {
	rb->getTransform().getOpenGLMatrix(mvp);
	return mvp;
}

void Model::draw(Engine::Shader *shader) {
    shader->uploadMat4("mvp", getMVP());
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

MeshData::MeshData() {
	vertices = new std::vector<float>();
	output = new std::vector<float>();
	normals = new std::vector<float>();
	indices = new std::vector<int>();
    texturePath = new char[128];
}

MeshData::~MeshData() {
	vertices->clear();
	vertices->shrink_to_fit();
	indices->clear();
	indices->shrink_to_fit();
	output->clear();
	output->shrink_to_fit();
	normals->clear();
	normals->shrink_to_fit();
}