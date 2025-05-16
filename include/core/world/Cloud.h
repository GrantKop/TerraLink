#ifndef CLOUD_H
#define CLOUD_H

#include "graphics/VertexArrayObject.h"

namespace std {
    template <>
    struct hash<glm::ivec2> {
        std::size_t operator()(const glm::ivec2& v) const noexcept {
            std::size_t h1 = std::hash<int>{}(v.x);
            std::size_t h2 = std::hash<int>{}(v.y);
            return h1 ^ (h2 << 1);
        }
    };
}

struct CloudMesh {
    VertexArrayObject VAO;
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    bool isUploaded = false;
};

using CloudPosition = glm::ivec2;

#endif
