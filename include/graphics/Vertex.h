#ifndef VERTEX_H
#define VERTEX_H

#include <iostream>
#include <string>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

struct UniformVertex {
    GLuint data;
};

#endif
