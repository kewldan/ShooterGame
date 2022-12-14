#include "Skybox.h"

#include "stb_image.h"

Skybox::Skybox(const char* filename)
{
    glGenTextures(1, &texture);
    bind();

	char* label = new char[256];
	strcpy(label, "Texture 3D [");
	strcat(label, filename);
	strcat(label, "]");
	glObjectLabelStr(GL_TEXTURE, texture, label);

    int width, height, nrChannels;
    char* path = new char[256];
    strcpy(path, "./data/textures/");
	strcat(path, filename);
	char n[2];
	n[1] = 0;
    for (unsigned int i = 0; i < 6; i++)
    {
		n[0] = 0x30 + i;
        strcat(path, n);
        strcat(path, ".jpg");
        unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
        }
        else
        {
            PLOGE << "Cubemap tex failed to load at path: " << path;
        }
        stbi_image_free(data);
        path[strlen(path) - 5] = 0;
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	std::vector<float> vertices = {
					-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};
	std::vector<int> indices = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35
	};

    mesh = new MyMesh(&vertices, &indices, 3);
    mesh->addParameter(0, 3);
}

Skybox::~Skybox()
{
    glDeleteTextures(1, &texture);
}

void Skybox::draw(Shader* shader, Camera* camera)
{
	glDepthFunc(GL_LEQUAL);
	shader->bind();
	shader->upload("proj", camera->getPerspective());
	shader->upload("view", camera->getViewRotationOnly());
	glActiveTexture(GL_TEXTURE0);
	shader->bind();

	shader->upload("skybox", 0);
	mesh->draw();
	glDepthFunc(GL_LESS);
}

void Skybox::bind()
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
}
