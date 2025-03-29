#ifndef chunk_h
#define chunk_h

#include <array>
#include <stdexcept>

#include "core/registers/BlockRegister.h"

constexpr int CHUNK_SIZE = 16;
constexpr int CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

class Chunk {
public:
    Chunk();
    ~Chunk();

    const Block& getBlock(int x, int y, int z);
    int getBlockID(int x, int y, int z);
    void setBlockID(int x, int y, int z, int blockID);

    void generateMesh(std::vector<Vertex>& vertices, std::vector<GLuint>& indices);

private:
    std::array<int, CHUNK_VOLUME> blocks;

    int index(int x, int y, int z);

    std::vector<Vertex> getFaceVertices(int face, const Block& block);
};

#endif
