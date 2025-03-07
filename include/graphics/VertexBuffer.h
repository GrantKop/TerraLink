#ifndef VAO_H
#define VAO_H

#include <iostream>
#include <glad/glad.h>

class VertexBuffer {
    public:
        VertexBuffer();
        ~VertexBuffer();

        GLuint VAO;
        GLuint VBO;
        GLuint EBO;

        template <typename T> void setData(GLintptr offset, GLuint numElements, T* data, GLenum usage = GL_STATIC_DRAW);
        template <typename T> void updateData(GLintptr offset, GLuint numElements, T* data);
        template <typename T> void setAttribPointer(GLuint idx, GLint size, GLenum type, GLsizei stride, GLuint divisor = 0);
        
        void draw(GLenum mode, GLsizei count, GLenum type, const void* indices, GLuint instanceCount = 1);

        void unbind() { glBindVertexArray(0); }

        void cleanup();

};

#endif
