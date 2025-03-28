#ifndef MESH_H
#define MESH_H

#include <iostream>
#include <string>
#include <vector>

#include "graphics/VertexArrayObject.h"
#include "graphics/Vertex.h"


class Mesh {
public:

    Mesh();
    virtual ~Mesh();

    void setVertices(std::vector<Vertex>& vertices, std::vector<GLuint>& indices);
    void uploadToGPU();
    void virtual draw();
    void cleanUp();

private:
    VertexArrayObject VAO;

    bool uploaded = false;

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

};

#endif
