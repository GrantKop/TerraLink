#ifndef MESH_H
#define MESH_H

#include <iostream>
#include <string>
#include <vector>

#include "VertexArrayObject.h"
#include "graphics/Vertex.h"
#include "graphics/Shader.h"
#include "graphics/Texture.h"
#include "core/camera/Camera.h"


class Mesh {
public:
    Shader* shader;
    VertexArrayObject VAO;
    Texture* texture;

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    Mesh(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, Shader* shader, Texture* texture = nullptr);
    ~Mesh();

    void draw(Camera& camera);
    void drawInstanced(int amount);

private:

};

#endif
