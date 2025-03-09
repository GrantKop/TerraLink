#ifndef VAO_H
#define VAO_H

#include <vector>
#include <glad/glad.h>

class VertexArrayObject {
    public:
        VertexArrayObject();
        ~VertexArrayObject();

        void bind();
        void unbind();
        void deleteBuffers();

        void addVertexBuffer(GLfloat* vertices, GLsizeiptr size, GLenum usage = GL_STATIC_DRAW);
        void addElementBuffer(GLuint* indices, GLsizeiptr size, GLenum usage = GL_STATIC_DRAW);
        void addAttribute(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);

    private:
        GLuint VAO, VBO, EBO;

};

#endif
