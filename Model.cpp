#include "Model.h"

void Model::processNode(aiNode* node, const aiScene* scene)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh, scene));
	}
	// then do the same for each of its children
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene);
	}
}

MyMesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{

		vertices.push_back(vertex);
	}

	return MyMesh(vertices, indices, );
}

Model::Model(std::string filename) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		PLOGE << "Failder to load model [" << filename << "]";
		return;
	}

	processNode(scene->mRootNode, scene);

	//mesh = new MyMesh(&output, indices, 8);
	//mesh->addParameter(0, 3);
	//mesh->addParameter(1, 2);
	//mesh->addParameter(2, 3);
}

Model::~Model()
{
	
}

glm::mat4 Model::getMVP() {
	mvp = glm::mat4(1.0f);
    //TODO: MVP
	return mvp;
}

void Model::draw()
{
	for (auto & mesh : meshes) {
		mesh.draw();
	}
}


