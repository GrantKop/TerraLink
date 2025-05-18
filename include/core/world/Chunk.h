#ifndef CHUNK_H
#define CHUNK_H

#include <array>
#include <stdexcept>
#include <chrono>

#include "core/registers/BlockRegister.h"
#include "graphics/VertexArrayObject.h"
#include "core/threads/ThreadSafeQueue.h"
#include "core/world/BiomeNoise.h"

constexpr int CHUNK_SIZE = 16;
constexpr int CHUNK_SIZE_P = CHUNK_SIZE + 2;
constexpr int CHUNK_VOLUME = CHUNK_SIZE_P * CHUNK_SIZE_P * CHUNK_SIZE_P;

struct ChunkPosition {
    int x, y, z;

    bool operator==(const ChunkPosition& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

};

// Hash function for ChunkPosition to be used in unordered_map
// Based on Boost's hash_combine
// https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine
namespace std {
    template <>
    struct hash<ChunkPosition> {
        size_t operator()(const ChunkPosition& pos) const {
            size_t chunk_seed = 0;
            chunk_seed ^= std::hash<int>()(pos.x) + 0x9e3779b9 + (chunk_seed << 6) + (chunk_seed >> 2);
            chunk_seed ^= std::hash<int>()(pos.y) + 0x9e3779b9 + (chunk_seed << 6) + (chunk_seed >> 2);
            chunk_seed ^= std::hash<int>()(pos.z) + 0x9e3779b9 + (chunk_seed << 6) + (chunk_seed >> 2);
            return chunk_seed;
        }
    };
}

struct ChunkMesh {
    VertexArrayObject VAO;
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    // Chunk mesh thread flags
    bool isUploaded = false;
    bool needsUpdate = false;
    bool isEmpty = true;
    std::vector<Vertex> stagingVertices;
    std::vector<GLuint> stagingIndices;
    std::atomic<bool> hasNewMesh = false;
};

class Chunk {
public:
    Chunk();
    ~Chunk();

    ChunkMesh mesh;

    glm::vec3 foliageColor = glm::vec3(0.0f, 0.0f, 0.0f);

    void generateTerrain();

    const Block& getBlock(int x, int y, int z) const;
    inline int getBlockID(int x, int y, int z) const {
        int idx = index(x, y, z);
        if (idx == -1) {
            std::cerr << "Chunk::getBlockID: index out of chunk bounds at " << x << ", " << y << ", " << z << std::endl;
            return -1;
        }
        return blocks[idx];
    }

    std::array<uint16_t, CHUNK_VOLUME> getBlocks() const {
        return blocks;
    }

    void setBlocks(const std::array<uint16_t, CHUNK_VOLUME>& newBlocks) {
        blocks = newBlocks;
    }
    
    void setBlockID(int x, int y, int z, int blockID);

    ChunkPosition getPosition() const;
    void setPosition(const ChunkPosition& pos);

    void generateMesh(std::vector<Vertex>& vertices, std::vector<GLuint>& indices,
        std::function<int(glm::ivec3 offset, int, int, int)> getBlockIDFromNeighbor) const;

    void addBlockFaceMesh(const Block& block, int x, int y, int z, int face,
                          std::vector<Vertex>& vertices, std::vector<GLuint>& indices,
                          GLuint& indexOffset, glm::vec3 chunkOffset) const;

    void addCoveredCrossMesh(const Block& block, int x, int y, int z,
                             std::vector<Vertex>& vertices, std::vector<GLuint>& indices,
                             GLuint& indexOffset, glm::vec3 chunkOffset) const;

private:
    std::array<uint16_t, CHUNK_VOLUME> blocks = {0};

    inline int index(int x, int y, int z) const {
        if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE)
            return -1;
        return x + (y * CHUNK_SIZE * CHUNK_SIZE) + (z * CHUNK_SIZE);
    }
    
    inline void getFaceVertices(int face, const Block& block, std::vector<Vertex>& vertices) const {
        int base = face * 4;
        for (int i = 0; i < 4; ++i)
            vertices.push_back(block.vertices[base + i]);
    }
    
    ChunkPosition position;
};

#endif
