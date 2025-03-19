#include "graphics/Mesh.h"

Mesh::Mesh(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, Shader* shader, Texture* texture) {
    this->vertices = vertices;
    this->indices = indices;
    this->shader = shader;
    this->texture = texture;

    VAO.bind();
    VAO.addVertexBuffer(vertices);
    VAO.addElementBuffer(indices);
    VAO.addAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    VAO.addAttribute(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    VAO.addAttribute(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    VAO.unbind();
}

Mesh::~Mesh() {}

void Mesh::draw(Camera& camera) {
    shader->use();
    shader->setUniform4("cameraMatrix", camera.cameraMatrix);
    shader->setUniform3("camPos", camera.position);

    if (texture != nullptr) texture->bind();
    
    VAO.bind();
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

void Mesh::drawInstanced(int amount) {
    shader->use();
    texture->bind();
    VAO.bind();
    glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, amount);
}
