#include "core/world/chunk.h"

Chunk::Chunk() {
    mesh.isEmpty = true;
}

Chunk::~Chunk() {}

// Generates terrain for the chunk using Perlin noise
void Chunk::generateTerrain(int seed, int octaves, float persistence, float lacunarity, float frequency, float amplitude) {
    int worldMinY = position.y * CHUNK_SIZE;
    int worldMaxY = (position.y + 1) * CHUNK_SIZE;

    bool hasTerrain = false;
    for (int x = 0; x < CHUNK_SIZE && !hasTerrain; ++x) {
        for (int z = 0; z < CHUNK_SIZE && !hasTerrain; ++z) {
            int worldX = position.x * CHUNK_SIZE + x;
            int worldZ = position.z * CHUNK_SIZE + z;
            float height = Noise::getHeight(worldX, worldZ, 0, 1, 0.5f, 2.0f, 0.01f, 16.0f);
            if (height >= worldMinY) {
                hasTerrain = true;
            }
        }
    }

    if (!hasTerrain) return;

    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int y = 0; y < CHUNK_SIZE; ++y) {
            for (int z = 0; z < CHUNK_SIZE; ++z) {
                int worldX = position.x * CHUNK_SIZE + x;
                int worldY = position.y * CHUNK_SIZE + y;
                int worldZ = position.z * CHUNK_SIZE + z; 

                float height = Noise::getHeight(worldX, worldZ, 0, 1, 0.5f, 2.0f, 0.01f, 16.0f);
                int maxY = static_cast<int>(height); 

                if (worldY < maxY - 3) {
                    setBlockID(x, y, z, BlockRegister::instance().blocks[1].ID);
                } else if (worldY < maxY && worldY >= maxY - 3) {
                    setBlockID(x, y, z, BlockRegister::instance().blocks[3].ID);
                } else if (worldY == maxY) {
                    setBlockID(x, y, z, BlockRegister::instance().blocks[2].ID);
                } else {
                    setBlockID(x, y, z, BlockRegister::instance().blocks[0].ID);
                }
            }
        }
    }
    
    mesh.isEmpty = false;
}

// Converts 3D coordinates to a 1D index for the blocks array
int Chunk::index(int x, int y, int z) const {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE) {
        return -1; // Out of bounds
    }
    return x + (y * CHUNK_SIZE * CHUNK_SIZE) + (z * CHUNK_SIZE);
}

// Retrieves the vertices for a specific face of the block
std::vector<Vertex> Chunk::getFaceVertices(int face, const Block& block) const {
    std::vector<Vertex> faceVertices;
    
    if (face == BACK) {
        faceVertices.push_back(block.vertices[0]);
        faceVertices.push_back(block.vertices[1]);
        faceVertices.push_back(block.vertices[2]);
        faceVertices.push_back(block.vertices[3]);
    } else if (face == BOTTOM) {
        faceVertices.push_back(block.vertices[4]);
        faceVertices.push_back(block.vertices[5]);
        faceVertices.push_back(block.vertices[6]);
        faceVertices.push_back(block.vertices[7]);
    } else if (face == FRONT) {
        faceVertices.push_back(block.vertices[8]);
        faceVertices.push_back(block.vertices[9]);
        faceVertices.push_back(block.vertices[10]);
        faceVertices.push_back(block.vertices[11]);
    } else if (face == LEFT) {
        faceVertices.push_back(block.vertices[12]);
        faceVertices.push_back(block.vertices[13]);
        faceVertices.push_back(block.vertices[14]);
        faceVertices.push_back(block.vertices[15]);
    } else if (face == RIGHT) {
        faceVertices.push_back(block.vertices[16]);
        faceVertices.push_back(block.vertices[17]);
        faceVertices.push_back(block.vertices[18]);
        faceVertices.push_back(block.vertices[19]);
    } else if (face == TOP) {
        faceVertices.push_back(block.vertices[20]);
        faceVertices.push_back(block.vertices[21]);
        faceVertices.push_back(block.vertices[22]);
        faceVertices.push_back(block.vertices[23]);
    }
    else {
        std::cerr << "Invalid face index: " << face << std::endl;
        return faceVertices; // Return empty vector if invalid face
    }
    return faceVertices;
}

// Retrieves a block from the blocks array using 3D coordinates
const Block& Chunk::getBlock(int x, int y, int z) const {
    int idx = index(x, y, z);
    if (idx == -1) {
        std::cerr << "Chunk::getBlock: index out of chunk bounds at " << x << ", " << y << ", " << z << std::endl;
        return BlockRegister::instance().blocks[0]; // Return air block if out of bounds
    }
    return BlockRegister::instance().blocks[blocks[idx]];
}

// Retrieves the block ID from the blocks array using 3D coordinates
int Chunk::getBlockID(int x, int y, int z) const {
    int idx = index(x, y, z);
    if (idx == -1) {
        std::cerr << "Chunk::getBlockID: index out of chunk bounds at " << x << ", " << y << ", " << z << std::endl;
        return -1;
    }
    return blocks[idx];
}

// Sets the block ID in the blocks array using 3D coordinates
void Chunk::setBlockID(int x, int y, int z, int blockID) {
    int idx = index(x, y, z);
    if (idx == -1) {
        std::cerr << "Chunk::setBlockID: index out of chunk bounds at " << x << ", " << y << ", " << z << std::endl;
        return;
    }
    blocks[idx] = blockID;
}

// Retrieves the chunk position
ChunkPosition Chunk::getPosition() const {
    return position;
}

// Sets the chunk position
void Chunk::setPosition(const ChunkPosition& pos) {
    position = pos;
}

// Generates the mesh for the chunk, considering neighboring chunks
void Chunk::generateMesh(std::vector<Vertex>& vertices, std::vector<GLuint>& indices,
                         std::function<int(glm::ivec3 offset, int, int, int)> getBlockIDFromNeighbor) const {
    vertices.clear();
    indices.clear();

    GLuint indexOffset = 0;

    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int y = 0; y < CHUNK_SIZE; ++y) {
            for (int z = 0; z < CHUNK_SIZE; ++z) {
                const Block block = getBlock(x, y, z);

                if (block.isAir) continue;

                for (int face = 0; face < 6; ++face) {
                    glm::ivec3 offset = FACE_OFFSETS[face];
                    int nx = x + offset.x;
                    int ny = y + offset.y;
                    int nz = z + offset.z;

                    int neighborBlockID;

                    // if neighbor block is out of bounds, check the neighboring chunk
                    // and add the face to the mesh if the neighbor is transparent
                    if (nx < 0 || nx >= CHUNK_SIZE
                     || ny < 0 || ny >= CHUNK_SIZE
                     || nz < 0 || nz >= CHUNK_SIZE) {
                        neighborBlockID = getBlockIDFromNeighbor(offset, x, y, z);
                        if (neighborBlockID == 0) {
                            const auto& faceVertices = getFaceVertices(face, block);
                            for (const auto& faceVertex : faceVertices) {
                                Vertex modifiedVertex = faceVertex;
                                glm::vec3 worldOffset = glm::vec3(position.x, position.y, position.z) * (float)CHUNK_SIZE;
                                modifiedVertex.position = worldOffset + glm::vec3(x, y, z) + faceVertex.position;
                                
                                vertices.push_back(modifiedVertex);
                            }

                            indices.insert(indices.end(), {
                                indexOffset, indexOffset + 2, indexOffset + 1,
                                indexOffset, indexOffset + 3, indexOffset + 2
                            });
                            indexOffset += 4;
                            continue;
                        }
                    } else {
                        neighborBlockID = getBlockID(nx, ny, nz);
                    }

                    if (!BlockRegister::instance().getBlockByIndex(neighborBlockID).isTransparent) continue;

                    const auto& faceVertices = getFaceVertices(face, block);
                    for (const auto& faceVertex : faceVertices) {
                        Vertex modifiedVertex = faceVertex;
                        glm::vec3 worldOffset = glm::vec3(position.x, position.y, position.z) * (float)CHUNK_SIZE;
                        modifiedVertex.position = worldOffset + glm::vec3(x, y, z) + faceVertex.position;

                        vertices.push_back(modifiedVertex);
                    }

                    indices.insert(indices.end(), {
                        indexOffset, indexOffset + 2, indexOffset + 1,
                        indexOffset, indexOffset + 3, indexOffset + 2
                    });                    
                    indexOffset += 4;
                }
            }
        }
    }
}
