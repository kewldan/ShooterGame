#ifndef SHOOTERGAME_MINIMAP_H
#define SHOOTERGAME_MINIMAP_H

#include "Shader.h"

class Minimap {
	int w, h, altitude;
	glm::vec3* pos;
	Shader* shader;
public:
	unsigned int FBO, map;
	Minimap(const char* shaderName, int width, int height, glm::vec3* position, int altitude);
	~Minimap();
	Shader* begin(float rotation_y);
	void end();
};

#endif