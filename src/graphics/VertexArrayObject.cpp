#include "graphics/VertexArrayObject.h"

VertexArrayObject::VertexArrayObject() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
}

VertexArrayObject::~VertexArrayObject() {}

void VertexArrayObject::bind() {
    glBindVertexArray(this->VAO);
}

void VertexArrayObject::unbind() {
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void VertexArrayObject::deleteBuffers() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void VertexArrayObject::addVertexBuffer(std::vector<Vertex>& vertices, GLenum usage) {
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), usage);
}

void VertexArrayObject::addElementBuffer(std::vector<GLuint>& indices, GLenum usage) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), usage);
}

void VertexArrayObject::addAttribute(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glVertexAttribPointer(index, size, type, normalized, stride, pointer);
    glEnableVertexAttribArray(index);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
