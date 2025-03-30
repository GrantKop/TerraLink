#include "core/world/FlatWorld.h"

void FlatWorld::generateChunks(int radius) {
    for (int x = -radius; x <= radius; ++x) {
        for (int z = -radius; z <= radius; ++z) {
            ChunkPosition pos = {x, 0, z};
            chunks[pos] = Chunk();
        }
    }
}

void FlatWorld::generateWorldMesh(std::vector<Vertex>& vertices, std::vector<GLuint>& indices) {
    vertices.clear();
    indices.clear();

    for (const auto& [pos, chunk] : chunks) {
        std::vector<Vertex> chunkVertices;
        std::vector<GLuint> chunkIndices;

        // pass in a lambda that queries the full world
        chunk.generateMesh(chunkVertices, chunkIndices, [&](glm::ivec3 offset, int x, int y, int z) -> int {
            if (offset.x == 0 && offset.y == 0 && offset.z == 0) {
                return chunk.getBlockID(x, y, z);
            } else {
                ChunkPosition neighborPos = {pos.x + offset.x, pos.y + offset.y, pos.z + offset.z};
                auto it = chunks.find(neighborPos);
                if (it != chunks.end()) {
                    return it->second.getBlockID(x, y, z);
                } else {
                    return BlockRegister::instance().getBlockByIndex(0).ID; // Air block
                }
            }
        });

        // Offset vertex positions to world space
        for (auto& v : chunkVertices) {
            v.position += glm::vec3(pos.x * CHUNK_SIZE, 0, pos.z * CHUNK_SIZE);
        }

        GLuint offset = static_cast<GLuint>(vertices.size());
        for (auto idx : chunkIndices) {
            indices.push_back(offset + idx);
        }

        vertices.insert(vertices.end(), chunkVertices.begin(), chunkVertices.end());
    }
}
