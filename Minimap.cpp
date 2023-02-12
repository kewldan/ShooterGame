#include "Minimap.h"

Minimap::Minimap(const char* shaderName, int width, int height, glm::vec3* position, int altitude)
{
	w = width;
	h = height;

	glGenTextures(1, &map);
	glBindTexture(GL_TEXTURE_2D, map);
	glObjectLabelStr(GL_TEXTURE, map, "Texture [Minimap]");
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
		w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glObjectLabelStr(GL_FRAMEBUFFER, FBO, "FBO [Minimap]");
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, map, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		PLOGE << "Minimap FBO invalid";
	}

	shader = new Shader(shaderName);
	this->pos = position;
	this->altitude = altitude;
}

Minimap::~Minimap()
{
	delete shader;
	glDeleteFramebuffers(1, &FBO);
	glDeleteTextures(1, &map);
}

Shader* Minimap::begin(float rotation_y)
{
	{
		rmt_ScopedCPUSample(Minimap_Preparing, 0);
		static const glm::mat4 proj = glm::ortho(-100.f, 100.f, -100.f, 100.f, 0.1f, 300.f);

		glm::vec2 rotation = glm::vec2(1.57f, rotation_y);

	    glm::mat4 view(1);
		view = glm::rotate(view, rotation.x, glm::vec3(1, 0, 0));
		view = glm::rotate(view, rotation.y, glm::vec3(0, 1, 0));
		view = glm::translate(view, -glm::vec3((*pos).x, 40 + (*pos).y, (*pos).z));
		shader->bind();
		shader->upload("proj", proj);
		shader->upload("view", view);
	}

	glViewport(0, 0, w, h);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glClearColor(0.5, 0.8, 1, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	return shader;
}

void Minimap::end()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
