#include "graphics/VertexBuffer.h"

VertexBuffer::VertexBuffer() {
    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &this->VBO);
    glGenBuffers(1, &this->EBO);
}

VertexBuffer::~VertexBuffer() {
    cleanup();
}

template <typename T> 
void VertexBuffer::setData(GLintptr offset, GLuint numElements, T* data, GLenum usage) {
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, numElements * sizeof(T), data, usage);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

template void VertexBuffer::setData<float>(int64_t, unsigned int, float*, unsigned int);

template <typename T> 
void VertexBuffer::updateData(GLintptr offset, GLuint numElements, T* data) {
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferSubData(GL_ARRAY_BUFFER, offset, numElements * sizeof(T), data);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

template <typename T> 
void VertexBuffer::setAttribPointer(GLuint idx, GLint size, GLenum type, GLsizei stride, GLuint divisor) {

    glBindVertexArray(this->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glVertexAttribPointer(idx, size, type, GL_FALSE, stride * sizeof(T), (void*)(0 * sizeof(T)));
    glEnableVertexAttribArray(idx);

    if (divisor != 0) glVertexAttribDivisor(idx, divisor);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

template void VertexBuffer::setAttribPointer<float>(unsigned int, int, unsigned int, int, unsigned int);

void VertexBuffer::draw(GLenum mode, GLsizei count, GLenum type, const void* indices, GLuint instanceCount) {
    glBindVertexArray(this->VAO);
    glDrawElementsInstanced(mode, count, type, (void*)indices, instanceCount);
    glBindVertexArray(0);
}

void VertexBuffer::cleanup() {
    glDeleteBuffers(1, &this->VBO);
    glDeleteBuffers(1, &this->EBO);
    glDeleteVertexArrays(1, &this->VAO);
}
