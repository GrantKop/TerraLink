#ifndef FLAT_WORLD_H
#define FLAT_WORLD_H

#include "core/world/Chunk.h"

class FlatWorld {
public:
    void generateChunks(int radius);
    void generateWorldMesh(std::vector<Vertex>& vertices, std::vector<GLuint>& indices);

private:
    std::unordered_map<ChunkPosition, Chunk, std::hash<ChunkPosition>> chunks;
};

#endif
