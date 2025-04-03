#ifndef chunk_h
#define chunk_h

#include <array>
#include <stdexcept>

#include "core/registers/BlockRegister.h"
#include "graphics/VertexArrayObject.h"
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
    int getBlockID(int x, int y, int z) const;
    void setBlockID(int x, int y, int z, int blockID);

    ChunkPosition getPosition() const;
    void setPosition(const ChunkPosition& pos);

    // Generates mesh for a single chunk, considering neighboring chunks when selecting faces
    void generateMesh(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, 
                      std::function<int(glm::ivec3 offset, int, int, int)> getBlockIDFromNeighbor) const;

private:
    std::array<int, CHUNK_VOLUME> blocks = {0};

    int index(int x, int y, int z) const;

    std::vector<Vertex> getFaceVertices(int face, const Block& block) const;

    ChunkPosition position;
};

#endif
