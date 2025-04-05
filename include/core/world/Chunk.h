#ifndef chunk_h
#define chunk_h

#include <array>
#include <stdexcept>
#include <chrono>

#include "core/registers/BlockRegister.h"
#include "graphics/VertexArrayObject.h"
#include "core/threads/ThreadSafeQueue.h"
#include "noise/Noise.h"

constexpr int CHUNK_SIZE = 16;
constexpr int CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

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
    bool needsUpdate = true;
    bool vaoInitialized = false;
    bool isEmpty = true;
};

class Chunk {
public:
    Chunk();
    ~Chunk();

    ChunkMesh mesh;

    void generateTerrain(int seed, int octaves, float persistence, float lacunarity, float frequency, float amplitude);

    const Block& getBlock(int x, int y, int z) const;
    inline int Chunk::getBlockID(int x, int y, int z) const {
        int idx = index(x, y, z);
        if (idx == -1) {
            std::cerr << "Chunk::getBlockID: index out of chunk bounds at " << x << ", " << y << ", " << z << std::endl;
            return -1;
        }
        return blocks[idx];
    }
    
    void setBlockID(int x, int y, int z, int blockID);

    ChunkPosition getPosition() const;
    void setPosition(const ChunkPosition& pos);

    void generateMesh(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, 
                      std::function<int(glm::ivec3 offset, int, int, int)> getBlockIDFromNeighbor) const;

private:
    std::array<uint16_t, CHUNK_VOLUME> blocks = {0};

    inline int Chunk::index(int x, int y, int z) const {
        if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE)
            return -1;
        return x + (y * CHUNK_SIZE * CHUNK_SIZE) + (z * CHUNK_SIZE);
    }
    
    inline void Chunk::getFaceVertices(int face, const Block& block, std::vector<Vertex>& vertices) const {
        int base = face * 4;
        for (int i = 0; i < 4; ++i)
            vertices.push_back(block.vertices[base + i]);
    }
    
    ChunkPosition position;
    
};

#endif
