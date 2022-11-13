#include "Model.h"

Model::Model(const char* filename, PhysicsWorld* world, PhysicsCommon* common, bool createConcaveCollider) {
	auto* indices = new std::vector<int>();
	auto* vertices = new std::vector<float>();
	auto* output = new std::vector<float>();

	loadMesh(filename, indices, vertices, output);

	new (this) Model(indices, vertices, output, world, common, createConcaveCollider);
}

Model::Model(std::vector<int>* indices, std::vector<float>* vertices, std::vector<float>* output, PhysicsWorld* world, PhysicsCommon* common, bool createConcaveCollider)
{
	myMesh = new MyMesh(output, indices, 8);
	myMesh->addParameter(0, 3);
	myMesh->addParameter(1, 2);
	myMesh->addParameter(2, 3);

	rb = world->createRigidBody(Transform::identity());

	if (createConcaveCollider) {
		rb->setType(BodyType::STATIC);
		TriangleVertexArray* triangleArray =
			new TriangleVertexArray(
				vertices->size() / 3, vertices->data(), 3 * sizeof(float),
				indices->size() / 3, indices->data(), 3 * sizeof(int),
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

void Model::loadMesh(const char* filename, std::vector<int>* indices, std::vector<float>* vertices, std::vector<float>* output)
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
		vertices->push_back(mesh->mVertices[i].x);
		vertices->push_back(mesh->mVertices[i].y);
		vertices->push_back(mesh->mVertices[i].z);

		output->push_back(mesh->mVertices[i].x);
		output->push_back(mesh->mVertices[i].y);
		output->push_back(mesh->mVertices[i].z);

		if (mesh->mTextureCoords[0])
		{
			output->push_back(mesh->mTextureCoords[0][i].x);
			output->push_back(mesh->mTextureCoords[0][i].y);
		}
		else {
			output->push_back(0);
			output->push_back(0);
		}

		if (mesh->HasNormals()) {
			output->push_back(mesh->mNormals[i].x);
			output->push_back(mesh->mNormals[i].y);
			output->push_back(mesh->mNormals[i].z);
		}
		else {
			output->push_back(0);
			output->push_back(0);
			output->push_back(0);
		}

	}

	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];

		indices->push_back(face.mIndices[0]);
		indices->push_back(face.mIndices[1]);
		indices->push_back(face.mIndices[2]);
	}
}

glm::mat4 Model::getMVP() {
	mvp = glm::mat4(1.0f);
	Transform transform = rb->getTransform();
	mvp = glm::translate(mvp, { transform.getPosition().x, transform.getPosition().y, transform.getPosition().z });
	Vector3 axis;
	float angle;
	transform.getOrientation().getRotationAngleAxis(angle, axis);
	if (angle != 0) {
		mvp = glm::rotate(mvp, angle, glm::vec3(axis.x, axis.y, axis.z));
	}
	return mvp;
}

void Model::draw() {
	myMesh->draw();
}
