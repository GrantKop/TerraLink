#ifndef TEXTURE_H
#define TEXTURE_H

#include <iostream>
#include <string>
#include <glad/glad.h>
#include <stb_image.h>

#include "graphics/Shader.h"

class Texture {

    public:
        Texture(const char* path, GLenum texType, GLenum texSlot, GLenum format, GLenum pixelType, GLenum minMagFilter = GL_NEAREST, GLenum wrapFilter = GL_REPEAT);
        ~Texture(); 

        GLenum slot, type, pixelType;
        GLuint ID, texUniform;
        int width, height, nrChannels;

        void bind();
        void unbind();
        void deleteTexture();

        void setUniform(Shader shader, const char* name, GLuint unit);
};


#endif
