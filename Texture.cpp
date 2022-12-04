#include "Texture.h"

Texture::Texture(const char* filename) {
	glGenTextures(1, &texture);

	bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	char* f = new char[256];
	strcpy(f, "./data/textures/");
	strcat(f, filename);

	unsigned char* data = stbi_load(f, &width, &height, &nrChannels, 0);
	if (data) {
		assert((void("Image channels must be 3 or 4!"), nrChannels == 3 || nrChannels == 4));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, nrChannels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE,
			data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glObjectLabelBuild(GL_TEXTURE, texture, "Texture", filename);

		PLOGI << "Texture [" << filename << "] loaded (" << width << "x" << height << ")";
	}
	else {
		PLOGE << "Failed to load texture [" << filename << "]";
		PLOGE << stbi_failure_reason();
	}
	stbi_image_free(data);
}

Texture::~Texture()
{
	glDeleteTextures(1, &texture);
}

void Texture::bind() const {
	glBindTexture(GL_TEXTURE_2D, texture);
}
