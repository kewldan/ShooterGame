#ifndef SHOOTERGAME_SHADOWSCASTER_H
#define SHOOTERGAME_SHADOWSCASTER_H

#include "glad/gl.h"
#include "Shader.h"

class ShadowsCaster {
    unsigned int map, FBO;
    int w, h;
    Shader *shader;
    glm::mat4 lightSpaceMatrix;
public:
    ShadowsCaster(int width, int height, const char *shaderName, float z_near, float z_far, glm::vec3 *position,
                  glm::vec3 *lookAt);
    ~ShadowsCaster();

    Shader *begin();

    void end();

    glm::mat4 *getLightSpaceMatrix();

    unsigned int getMap() const;
};


#endif //SHOOTERGAME_SHADOWSCASTER_H
