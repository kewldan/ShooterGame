#ifndef SHOOTERGAME_SHADOWSCASTER_H
#define SHOOTERGAME_SHADOWSCASTER_H

#include "glad/glad.h"
#include "Shader.h"

class ShadowsCaster {
	unsigned int map, FBO;
	int w, h;
	Shader* shader;
	glm::mat4 lightSpaceMatrix;
	glm::vec3 position;
public:
	ShadowsCaster(int width, int height, const char* shaderName, glm::vec3 position);
	~ShadowsCaster();

	Shader* begin(glm::vec3 cam, float distance);

	void end();

	glm::mat4 getLightSpaceMatrix();

	unsigned int getMap() const;
};


#endif //SHOOTERGAME_SHADOWSCASTER_H
