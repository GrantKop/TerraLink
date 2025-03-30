#include "core/world/chunk.h"

Chunk::Chunk() {
    blocks.fill(1); // Initialize all blocks to air
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            setBlockID(x, CHUNK_SIZE - 1, z, 2); // top layer (e.g., grass)
            setBlockID(x, CHUNK_SIZE - 2, z, 3); // dirt
            setBlockID(x, CHUNK_SIZE - 3, z, 3); // dirt
            setBlockID(x, CHUNK_SIZE - 4, z, 3); // dirt
        }
    }
}

Chunk::~Chunk() {}

// Converts 3D coordinates to a 1D index for the blocks array
int Chunk::index(int x, int y, int z) {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE) {
        return -1; // Out of bounds
    }
    return x + (y * CHUNK_SIZE * CHUNK_SIZE) + (z * CHUNK_SIZE);
}

// Retrieves the vertices for a specific face of the block
std::vector<Vertex> Chunk::getFaceVertices(int face, const Block& block) {
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
const Block& Chunk::getBlock(int x, int y, int z) {
    int idx = index(x, y, z);
    if (idx == -1) {
        std::cerr << "Chunk::getBlock: index out of chunk bounds at " << x << ", " << y << ", " << z << std::endl;
        return BlockRegister::instance().blocks[0]; // Return air block if out of bounds
    }
    return BlockRegister::instance().blocks[blocks[idx]];
}

// Retrieves the block ID from the blocks array using 3D coordinates
int Chunk::getBlockID(int x, int y, int z) {
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

// Generates the mesh for the chunk based on the blocks
void Chunk::generateMesh(std::vector<Vertex>& vertices, std::vector<GLuint>& indices) {
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

                    // if neighbor is out of bounds, add the face to the mesh
                    if (nx < 0 || nx >= CHUNK_SIZE || ny < 0 || ny >= CHUNK_SIZE || nz < 0 || nz >= CHUNK_SIZE) {
                        const auto& faceVertices = getFaceVertices(face, block);
                        for (const auto& faceVertex : faceVertices) {
                            Vertex modifiedVertex = faceVertex;
                            modifiedVertex.position = glm::vec3(x, y, z) + faceVertex.position;
                            vertices.push_back(modifiedVertex);
                        }

                        indices.insert(indices.end(), {
                            indexOffset, indexOffset + 2, indexOffset + 1,
                            indexOffset, indexOffset + 3, indexOffset + 2
                        });                        
                        indexOffset += 4;
                        continue;
                    }

                    if (!BlockRegister::instance().getBlockByIndex(getBlockID(nx, ny, nz)).isTransparent) continue;

                    const auto& faceVertices = getFaceVertices(face, block);
                    for (const auto& faceVertex : faceVertices) {
                        Vertex modifiedVertex = faceVertex;
                        modifiedVertex.position = glm::vec3(x, y, z) + faceVertex.position;
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
