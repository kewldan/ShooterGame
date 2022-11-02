#ifndef SHOOTERGAME_TEXTURE_H
#define SHOOTERGAME_TEXTURE_H

#include "glad/gl.h"
#include <string>
#include "plog/Log.h"

class Texture {
    int width, height, nrChannels;
    unsigned int texture;
public:
    Texture(const char *filename);

    void bind() const;

    static void unbind();
};


#endif //SHOOTERGAME_TEXTURE_H
