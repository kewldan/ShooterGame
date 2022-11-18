#ifndef SHOOTERGAME_TEXTURE_H
#define SHOOTERGAME_TEXTURE_H

#include "glad/gl.h"
#include <string>
#include "plog/Log.h"

#include "stb_image.h"

class Texture {
	int width, height, nrChannels;
	unsigned int texture;
public:
	Texture(const char* filename);
	~Texture();

	void bind() const;
};


#endif //SHOOTERGAME_TEXTURE_H
