#include "Model.h"

Model::Model(std::string filename, PhysicsWorld* world, PhysicsCommon* common, bool createConcaveCollider) {
	this->world = world;

	indices = new std::vector<int>();
	vertices = new std::vector<float>();

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
	{
		PLOGE << "Failed to load " << filename;
		return;
	}

	aiNode* root = scene->mRootNode;
	aiNode* child = root->mChildren[0];
	aiMesh* mesh = scene->mMeshes[child->mMeshes[0]];

	auto* output = new std::vector<float>();
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

void Model::print()
{
	for (int i = 0; i < vertices->size() / 3; i++) {
		PLOGD << "V" << i << ' ' << vertices->data()[i * 3] << ", " << vertices->data()[i * 3 + 1] << ", " << vertices->data()[i * 3 + 2];
	}

	for (int i = 0; i < indices->size() / 3; i++) {
		PLOGD << "I" << i << ' ' << indices->data()[i * 3] << ", " << indices->data()[i * 3 + 1] << ", " << indices->data()[i * 3 + 2];
	}
}
