#include "Model.h"

Model::Model(const char* filename, PhysicsWorld* world, PhysicsCommon* common, bool createConcaveCollider) {
	auto* data = new MeshData();

	loadMesh(filename, data);

	new (this) Model(data, world, common, createConcaveCollider);
}

Model::Model(MeshData* data, PhysicsWorld* world, PhysicsCommon* common, bool createConcaveCollider)
{
	myMesh = new MyMesh(data->output, data->indices, 8);
	myMesh->addParameter(0, 3);
	myMesh->addParameter(1, 2);
	myMesh->addParameter(2, 3);

	rb = world->createRigidBody(Transform::identity());

	mvp = new float[16];

	if (createConcaveCollider) {
		rb->setType(BodyType::STATIC);
		TriangleVertexArray* triangleArray =
			new TriangleVertexArray(
				data->vertices->size() / 3, data->vertices->data(), 3 * sizeof(float),
				data->indices->size() / 3, data->indices->data(), 3 * sizeof(int),
				TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE,
				TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE);
		TriangleMesh* triangleMesh = common->createTriangleMesh();
		triangleMesh->addSubpart(triangleArray);
		rb->addCollider(common->createConcaveMeshShape(triangleMesh), Transform::identity());
	}
}

Model::~Model()
{
	delete myMesh;
}

void Model::loadMesh(const char* filename, MeshData* out)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
	{
		PLOGE << "Failed to load " << filename;
		return;
	}

	aiNode* root = scene->mRootNode;
	aiNode* child = root->mChildren[0];
	aiMesh* mesh = scene->mMeshes[child->mMeshes[0]];

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		out->vertices->push_back(mesh->mVertices[i].x);
		out->vertices->push_back(mesh->mVertices[i].y);
		out->vertices->push_back(mesh->mVertices[i].z);

		out->output->push_back(mesh->mVertices[i].x);
		out->output->push_back(mesh->mVertices[i].y);
		out->output->push_back(mesh->mVertices[i].z);

		if (mesh->mTextureCoords[0])
		{
			out->output->push_back(mesh->mTextureCoords[0][i].x);
			out->output->push_back(mesh->mTextureCoords[0][i].y);
		}
		else {
			out->output->push_back(0);
			out->output->push_back(0);
		}

		if (mesh->HasNormals()) {
			out->output->push_back(mesh->mNormals[i].x);
			out->output->push_back(mesh->mNormals[i].y);
			out->output->push_back(mesh->mNormals[i].z);
		}
		else {
			out->output->push_back(0);
			out->output->push_back(0);
			out->output->push_back(0);
		}

	}

	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];

		out->indices->push_back(face.mIndices[0]);
		out->indices->push_back(face.mIndices[1]);
		out->indices->push_back(face.mIndices[2]);
	}

	PLOGI << "Loaded mesh from " << filename << "";
}

float* Model::getMVP() {
	rb->getTransform().getOpenGLMatrix(mvp);
	return mvp;
}

void Model::draw() {
	myMesh->draw();
}

MeshData::MeshData() {
	vertices = new std::vector<float>();
	output = new std::vector<float>();
	indices = new std::vector<int>();
}

MeshData::~MeshData() {
	vertices->clear();
	vertices->shrink_to_fit();
	indices->clear();
	indices->shrink_to_fit();
	output->clear();
	output->shrink_to_fit();
}