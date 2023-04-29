#ifndef SHOOTERGAME_SHADOWSCASTER_H
#define SHOOTERGAME_SHADOWSCASTER_H

#include "glad/glad.h"
#include "Shader.h"

class ShadowsCaster {
	unsigned int map, FBO;
	int w, h;
    Engine::Shader* shader;
	glm::mat4 lightSpaceMatrix, proj, view;
	glm::vec3 position;
public:
	ShadowsCaster(int width, int height, const char* shaderName, glm::vec3 position, float distance);
	~ShadowsCaster();

    Engine::Shader* begin(glm::vec3 cam);

	void end();

	glm::mat4 getLightSpaceMatrix();

	unsigned int getMap() const;
};


#endif //SHOOTERGAME_SHADOWSCASTER_H
