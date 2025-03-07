#include "graphics/VertexArrayObject.h"

VertexArrayObject::VertexArrayObject() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
}

VertexArrayObject::~VertexArrayObject() {
    deleteBuffers();
}

void VertexArrayObject::bind() {
    glBindVertexArray(VAO);
}

void VertexArrayObject::unbind() {
    glBindVertexArray(0);
}

void VertexArrayObject::deleteBuffers() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void VertexArrayObject::addVertexBuffer(GLfloat* vertices, GLsizeiptr size, GLenum usage) {
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, usage);
}

void VertexArrayObject::addElementBuffer(GLuint* indices, GLsizeiptr size, GLenum usage) {
    glBindVertexArray(VAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, usage);
}

void VertexArrayObject::addAttribute(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {
    glBindVertexArray(VAO);
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, size, type, normalized, stride, pointer);
    attributes.push_back(index);
}
