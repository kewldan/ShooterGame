#include "Model.h"

Model::Model(const char* filename, PhysicsWorld* world, PhysicsCommon* common, bool createConcaveCollider, char* label) {
	int nb = -2;
	MeshData* meshesData = loadMesh(filename, &nb);
	char* f = new char[256];
	strcpy(f, filename);
	new (this) Model(meshesData, nb, world, common, createConcaveCollider, label != nullptr ? label : f);
}

Model::Model(MeshData* data, int nb, PhysicsWorld* world, PhysicsCommon* common, bool createConcaveCollider, char* label)
{
	meshes = (MyMesh*)calloc(nb, sizeof(MyMesh));
	for (int i = 0; i < nb; i++) {
		meshes[i] = MyMesh(data[i].output, data[i].indices, 8, label);
		meshes[i].addParameter(0, 3);
		meshes[i].addParameter(1, 2);
		meshes[i].addParameter(2, 3);
		if (strlen(data[i].texturePath) > 0) {
			char* path = new char[256];
			strcpy(path, ".\\data\\textures\\");
			strcat(path, data[i].texturePath);
			meshes[i].texture = new Texture(path);
		}
	}
	nbMeshes = nb;

	rb = world->createRigidBody(Transform::identity());

	mvp = new float[16];

	if (createConcaveCollider) {
		rb->setType(BodyType::STATIC);
		TriangleMesh* triangleMesh = common->createTriangleMesh();
		for (int i = 0; i < nb; i++) {
			TriangleVertexArray* triangleArray =
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
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filename, aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);

	const char* error = aiGetErrorString();
	if (strlen(error) > 0) {
		PLOGE << error;
	}

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
	{
		PLOGE << "Failed to load " << filename;
		return nullptr;
	}

	aiNode* root = scene->mRootNode;
	aiNode* child = root->mChildren[0];

	MeshData* data = new MeshData[child->mNumMeshes];
	for (unsigned int j = 0; j < child->mNumMeshes; j++) {
		aiMesh* mesh = scene->mMeshes[child->mMeshes[j]];
		data[j] = MeshData();
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString str;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &str);
			strcpy(data[j].texturePath, str.C_Str());
		}
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			data[j].vertices->push_back(mesh->mVertices[i].x);
			data[j].vertices->push_back(mesh->mVertices[i].y);
			data[j].vertices->push_back(mesh->mVertices[i].z);

			data[j].output->push_back(mesh->mVertices[i].x);
			data[j].output->push_back(mesh->mVertices[i].y);
			data[j].output->push_back(mesh->mVertices[i].z);

			if (mesh->mTextureCoords[0])
			{
				data[j].output->push_back(mesh->mTextureCoords[0][i].x);
				data[j].output->push_back(mesh->mTextureCoords[0][i].y);
			}
			else {
				data[j].output->push_back(0);
				data[j].output->push_back(0);
			}

			data[j].output->push_back(mesh->mNormals[i].x);
			data[j].output->push_back(mesh->mNormals[i].y);
			data[j].output->push_back(mesh->mNormals[i].z);

			data[j].normals->push_back(mesh->mNormals[i].x);
			data[j].normals->push_back(mesh->mNormals[i].y);
			data[j].normals->push_back(mesh->mNormals[i].z);
		}

		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];

			data[j].indices->push_back(face.mIndices[0]);
			data[j].indices->push_back(face.mIndices[1]);
			data[j].indices->push_back(face.mIndices[2]);
		}
	}

	*len = child->mNumMeshes;

	PLOGI << "Model [" << filename << "] loaded " << child->mNumMeshes << " meshes";
	return data;
}

float* Model::getMVP() {
	rb->getTransform().getOpenGLMatrix(mvp);
	return mvp;
}

MeshData::MeshData() {
	vertices = new std::vector<float>();
	output = new std::vector<float>();
	normals = new std::vector<float>();
	indices = new std::vector<int>();
	texturePath = new char[256];
	strcpy(texturePath, "");
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