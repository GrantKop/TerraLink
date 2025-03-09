#ifndef TEXTURE_H
#define TEXTURE_H

#include <iostream>
#include <string>
#include <glad/glad.h>
#include <stb_image.h>

#include "graphics/Shader.h"

// Texture object for ease of loading textures with STB image
class Texture {

    public:
        Texture(const char* path, GLenum texType, GLuint texSlot, GLenum format, GLenum pixelType, GLenum minMagFilter = GL_NEAREST, GLenum wrapFilter = GL_REPEAT);
        ~Texture(); 

        // Texture info for openGL
        GLenum type, pixelType;
        GLuint slot, ID, texUniform;
        int width, height, nrChannels;

        // Binds the texture object in openGL
        void bind();
        // Unbinds the texture object
        void unbind();
        // Deletes the texture from GL memory
        void deleteTexture();

        // Sets the uniform texture variable for use in shader programs
        void setUniform(Shader shader, const char* name, GLuint unit);
};

#endif
