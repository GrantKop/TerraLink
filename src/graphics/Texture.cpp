#include "graphics/Texture.h"

Texture::Texture(const char* path, GLenum texType, GLuint texSlot, GLenum format, GLenum pixelType, GLenum minMagFilter, GLenum wrapFilter) {
    
    type = texType;
    slot = texSlot;

    stbi_set_flip_vertically_on_load(true);
    unsigned char* imageData = stbi_load(path, &width, &height, &nrChannels, 0);

    glGenTextures(1, &ID);
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(texType, ID);

    glTexParameteri(texType, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(texType, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(texType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(texType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glTexImage2D(texType, 0, GL_RGBA, width, height, 0, format, pixelType, imageData);

    stbi_image_free(imageData);
    glBindTexture(texType, 0);
    
}

Texture::~Texture() {}

void Texture::bind() {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(type, ID);
}

void Texture::unbind() {
    glBindTexture(type, 0);
}

void Texture::deleteTexture() {
    glDeleteTextures(1, &ID);
}

void Texture::setUniform(Shader shader, const char* name, GLuint unit) {
    texUniform = glGetUniformLocation(shader.ID, name);
    shader.use();
    glUniform1i(texUniform, unit);
}
