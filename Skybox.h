#ifndef SHOOTERGAME_SKYBOX_H
#define SHOOTERGAME_SKYBOX_H

#include "glad/glad.h"
#include "plog/Log.h"
#include "MyMesh.h"
#include "Shader.h"


class Skybox {
	int width, height, nrChannels;
	unsigned int texture;
	MyMesh* mesh;
public:
	Skybox(const char* filename);
	~Skybox();

	void draw(Shader* shader, Camera* camera);

	void bind();
};

#endif