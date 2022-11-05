#include "Model.h"
#include "OBJ_Loader.h"


Model::Model(std::string filename, PhysicsWorld* world, PhysicsCommon* common, bool createConcaveCollider) {
	std::vector<float> output;

	indices = new std::vector<int>();
	vertices = new std::vector<float>();

	objl::Loader Loader;
	Loader.LoadFile(filename);
	objl::Mesh curMesh = Loader.LoadedMeshes[0];
	for (auto& vertex : curMesh.Vertices) {
		output.push_back(vertex.Position.X);
		output.push_back(vertex.Position.Y);
		output.push_back(vertex.Position.Z);

		vertices->push_back(vertex.Position.X);
		vertices->push_back(vertex.Position.Y);
		vertices->push_back(vertex.Position.Z);

		output.push_back(vertex.TextureCoordinate.X);
		output.push_back(vertex.TextureCoordinate.Y);

		output.push_back(vertex.Normal.X);
		output.push_back(vertex.Normal.Y);
		output.push_back(vertex.Normal.Z);
	}

	for (int j = 0; j < curMesh.Indices.size(); j += 3) {
		indices->push_back(curMesh.Indices[j]);
		indices->push_back(curMesh.Indices[j + 1]);
		indices->push_back(curMesh.Indices[j + 2]);
	}

	mesh = new MyMesh(&output, indices, 8);
	mesh->addParameter(0, 3);
	mesh->addParameter(1, 2);
	mesh->addParameter(2, 3);

	rb = world->createRigidBody(Transform::identity());

	if (createConcaveCollider) {
		rb->setType(BodyType::STATIC);
		TriangleVertexArray* triangleArray =
			new TriangleVertexArray(vertices->size() / 3, vertices->data(), 3 * sizeof(
				float), indices->size() / 3,
				indices->data(), 3 * sizeof(int),
				TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE,
				TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE);
		TriangleMesh *triangleMesh = common->createTriangleMesh();
		triangleMesh->addSubpart(triangleArray);
		rb->addCollider(common->createConcaveMeshShape(triangleMesh), Transform::identity());
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

MyMesh* Model::getMesh() const {
	return mesh;
}
