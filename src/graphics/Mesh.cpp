#include "graphics/Mesh.h"

Mesh::Mesh() {}

Mesh::~Mesh() {
    cleanUp();
}

void Mesh::setVertices(std::vector<Vertex>& vertices, std::vector<GLuint>& indices) {
    this->vertices = vertices;
    this->indices = indices;
}

void Mesh::uploadToGPU() {

    if (uploaded) return;

    VAO.bind();
    VAO.addVertexBuffer(vertices);
    VAO.addElementBuffer(indices);
    VAO.addAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    VAO.addAttribute(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    VAO.addAttribute(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    uploaded = true;
}

void Mesh::draw() {

    if (!uploaded) {
        std::cerr << "Mesh not uploaded to GPU" << std::endl;
        return;
    }

    VAO.bind();
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

void Mesh::cleanUp() {
    VAO.deleteBuffers();
}