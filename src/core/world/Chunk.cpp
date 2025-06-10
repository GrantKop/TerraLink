#include "core/world/Chunk.h"

#include <random>

Chunk::Chunk() {
    mesh.isEmpty = true;
}

Chunk::~Chunk() {}

void Chunk::generateTerrain() {
    int worldMinY = position.y * CHUNK_SIZE;
    int worldMaxY = worldMinY + CHUNK_SIZE;

    if (BiomeNoise::isChunkLikelyEmpty(position)) {
        mesh.isEmpty = true;
        return;
    }
    
    bool hasSurface = false;

    for (int x = 0; x < CHUNK_SIZE && !hasSurface; ++x) {
        for (int z = 0; z < CHUNK_SIZE && !hasSurface; ++z) {
            int wx = position.x * CHUNK_SIZE + x;
            int wz = position.z * CHUNK_SIZE + z;

            float height = BiomeNoise::generateBlendedHeight(wx, wz);
            int groundHeight = static_cast<int>(height);

            if (groundHeight >= worldMinY - 1 && groundHeight < worldMaxY + 1) {
                hasSurface = true;
            }
        }
    }

    if (!hasSurface) {
        mesh.isEmpty = true;
        return;
    }

    std::mt19937 rng(position.x * 73856093 ^ position.y * 19349663 ^ position.z * 83492791);
    std::uniform_real_distribution<float> scatterChance(0.0f, 1.0f);

    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            int wx = position.x * CHUNK_SIZE + x;
            int wz = position.z * CHUNK_SIZE + z;

            float height = BiomeNoise::generateBlendedHeight(wx, wz);
            int groundHeight = static_cast<int>(height);

            auto blends = BiomeNoise::getBiomeBlend(wx, wz);
            int dominantBiome = blends[0].first;

            for (int y = 0; y < CHUNK_SIZE; ++y) {
                int worldY = position.y * CHUNK_SIZE + y;

                if (worldY > groundHeight) continue;

                float roll = scatterChance(rng);

                if (dominantBiome == _DESERT) {
                    setBlockID(x, y, z, worldY < groundHeight - 3 ? 1 : 7);
                    int aboveY = y + 1;
                    if (aboveY < CHUNK_SIZE && getBlock(x, aboveY, z).isAir) {
                        if (roll < 0.0085f) {
                            setBlockID(x, aboveY, z, 13);
                        } else if (roll > 0.999f) {
                            setBlockID(x, aboveY, z, 12);
                            if (aboveY + 1 < CHUNK_SIZE && getBlock(x, aboveY + 1, z).isAir) setBlockID(x, aboveY + 1, z, 12);
                        }
                    }
                } else if (dominantBiome == _OCEAN || dominantBiome == _RIVER) {
                    setBlockID(x, y, z, worldY < groundHeight - 2 ? 1 : 7);
                } else if (dominantBiome == _MOUNTAINS) {
                    if (worldY >= groundHeight - 4 && worldY > 95) {
                        if (worldY > 135)
                            setBlockID(x, y, z, 8);
                        else {
                            setBlockID(x, y, z, 1);
                        }
                    } else {
                        if (worldY < groundHeight - 4)
                            setBlockID(x, y, z, 1);
                        else if (worldY < groundHeight)
                            setBlockID(x, y, z, 3);
                        else {
                            setBlockID(x, y, z, 2);
                            int aboveY = y + 1;
                            if (aboveY < CHUNK_SIZE && getBlock(x, aboveY, z).isAir) {
                                if (roll < 0.08f) {
                                    setBlockID(x, aboveY, z, 11);
                                }
                            }
                        }
                    }
                } else {
                    if (worldY < groundHeight - 4)
                        setBlockID(x, y, z, 1);
                    else if (worldY < groundHeight)
                        setBlockID(x, y, z, 3);
                    else {
                        setBlockID(x, y, z, 2);

                        int aboveY = y + 1;
                        if (aboveY < CHUNK_SIZE && getBlock(x, aboveY, z).isAir) {
                        
                            if (roll < 0.08f) {
                                setBlockID(x, aboveY, z, 11);
                            }
                        
                            bool isForest = dominantBiome == _FOREST;
                            bool isPlains = dominantBiome == _PLAINS;
                            bool treeChance = (isForest && roll < 0.065f) || (isPlains && roll < 0.022f);
                        
                            if (treeChance) {
                                bool canPlace = true;
                                for (int dy = 0; dy < 3; ++dy) {
                                    int trunkY = y + 1 + dy;
                                    if (trunkY >= CHUNK_SIZE) continue;
                                    if (!getBlock(x, trunkY, z).isAir) {
                                        canPlace = false;
                                        break;
                                    }
                                }
                            
                                if (canPlace) {
                                    for (int dy = 0; dy < 3; ++dy) {
                                        int trunkY = y + 1 + dy;
                                        if (trunkY >= CHUNK_SIZE) continue;
                                        setBlockID(x, trunkY, z, 4);
                                    }
                                
                                    int leafBaseY = y + 3;
                                    for (int dx = -1; dx <= 1; ++dx) {
                                        for (int dy = 0; dy <= 2; ++dy) {
                                            for (int dz = -1; dz <= 1; ++dz) {
                                                int lx = x + dx;
                                                int ly = leafBaseY + dy;
                                                int lz = z + dz;
                                                if (lx < 0 || lx >= CHUNK_SIZE ||
                                                    ly < 0 || ly >= CHUNK_SIZE ||
                                                    lz < 0 || lz >= CHUNK_SIZE) {
                                                    continue;
                                                }
                                                if (getBlock(lx, ly, lz).isAir) {
                                                    setBlockID(lx, ly, lz, 6);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    mesh.isEmpty = false;
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
                         std::function<int(glm::ivec3 offset, int, int, int)> getBlockIDFromNeighbor) const
{
    thread_local std::vector<bool> isTransparentCache;
    thread_local const std::vector<Block>* blockListPtr = &BlockRegister::instance().blocks;

    // Ensure the transparency cache is up-to-date
    if (isTransparentCache.size() != blockListPtr->size()) {
        isTransparentCache.resize(blockListPtr->size());
        for (size_t i = 0; i < blockListPtr->size(); ++i) {
            isTransparentCache[i] = (*blockListPtr)[i].isTransparent;
        }
    }

    GLuint indexOffset = 0;
    glm::vec3 chunkOffset = glm::vec3(position.x, position.y, position.z) * (float)CHUNK_SIZE;

    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int y = 0; y < CHUNK_SIZE; ++y) {
            for (int z = 0; z < CHUNK_SIZE; ++z) {
                int blockID = getBlockID(x, y, z);
                if (blockID <= 0 || blockID >= (int)blockListPtr->size()) continue;

                const Block& block = (*blockListPtr)[blockID];
                if (block.isAir) continue;

                bool transparent = block.isTransparent;

                if (block.model == "covered_cross") {
                    addCoveredCrossMesh(
                        block, x, y, z,
                        vertices, indices,
                        indexOffset,
                        chunkOffset
                    );
                    continue;
                }

                if (block.model == "cross") {
                    addCrossMesh(
                        block, x, y, z,
                        vertices, indices,
                        indexOffset,
                        chunkOffset
                    );
                    continue;
                }

                for (int face = 0; face < 6; ++face) {
                    glm::ivec3 offset = FACE_OFFSETS[face];
                    int nx = x + offset.x;
                    int ny = y + offset.y;
                    int nz = z + offset.z;

                    int neighborID = -1;
                    if (nx < 0 || nx >= CHUNK_SIZE ||
                        ny < 0 || ny >= CHUNK_SIZE ||
                        nz < 0 || nz >= CHUNK_SIZE) {
                        neighborID = getBlockIDFromNeighbor(offset, x, y, z);
                    } else {
                        neighborID = getBlockID(nx, ny, nz);
                    }

                    if (neighborID < 0 || neighborID >= (int)isTransparentCache.size()) continue;
                    if (!isTransparentCache[neighborID] || neighborID == blockID) continue;

                    addBlockFaceMesh(
                        block, x, y, z, face,
                        vertices, indices,
                        indexOffset,
                        chunkOffset
                    );
                }
            }
        }
    }
}

void Chunk::addBlockFaceMesh(const Block& block, int x, int y, int z, int face,
                              std::vector<Vertex>& vertices, std::vector<GLuint>& indices,
                              GLuint& indexOffset, glm::vec3 chunkOffset) const {
    std::vector<Vertex> faceVerts;
    getFaceVertices(face, block, faceVerts);

    for (auto& vtx : faceVerts) {
        Vertex v = vtx;
        v.position += chunkOffset + glm::vec3(x, y, z);
        vertices.push_back(v);
    }

    indices.insert(indices.end(), {
        indexOffset, indexOffset + 2, indexOffset + 1,
        indexOffset, indexOffset + 3, indexOffset + 2
    });
    indexOffset += 4;
}

void Chunk::addCoveredCrossMesh(const Block& block, int x, int y, int z,
                              std::vector<Vertex>& vertices, std::vector<GLuint>& indices,
                              GLuint& indexOffset, glm::vec3 chunkOffset) const {
    int modelVertexCount = (int)block.vertices.size();
    if (modelVertexCount % 4 != 0) {
        std::cerr << "Block model mesh is not quad-based: " << modelVertexCount << " verts" << std::endl;
        return;
    }

    for (const auto& vtx : block.vertices) {
        Vertex v = vtx;
        v.position += chunkOffset + glm::vec3(x, y, z);
        vertices.push_back(v);
    }

    for (GLuint i = 0; i < modelVertexCount; i += 4) {
        indices.insert(indices.end(), {
            indexOffset + i, indexOffset + i + 2, indexOffset + i + 1,
            indexOffset + i, indexOffset + i + 3, indexOffset + i + 2
        });
    }

    indexOffset += modelVertexCount;
}

void Chunk::addCrossMesh(const Block& block, int x, int y, int z,
                         std::vector<Vertex>& vertices, std::vector<GLuint>& indices,
                         GLuint& indexOffset, glm::vec3 chunkOffset) const {
    int modelVertexCount = static_cast<int>(block.vertices.size());
    if (modelVertexCount % 4 != 0) {
        std::cerr << "Block model mesh is not quad-based: " << modelVertexCount << " verts" << std::endl;
        return;
    }

    // Compute world coordinates
    int worldX = x + position.x * CHUNK_SIZE;
    int worldY = y + position.y * CHUNK_SIZE;
    int worldZ = z + position.z * CHUNK_SIZE;

    // Consistent seed for this block position
    size_t seed = std::hash<int>()(worldX) ^ (std::hash<int>()(worldY) << 1) ^ (std::hash<int>()(worldZ) << 2);
    std::mt19937 rng(static_cast<unsigned int>(seed));
    std::uniform_real_distribution<float> offsetDist(-0.25f, 0.25f);

    float offsetX = offsetDist(rng);
    float offsetZ = offsetDist(rng);

    glm::vec3 posOffset = glm::vec3(x + offsetX, y, z + offsetZ);

    for (const auto& vtx : block.vertices) {
        Vertex v = vtx;
        v.position += chunkOffset + posOffset;
        vertices.push_back(v);
    }

    for (GLuint i = 0; i < modelVertexCount; i += 4) {
        indices.insert(indices.end(), {
            indexOffset + i, indexOffset + i + 2, indexOffset + i + 1,
            indexOffset + i, indexOffset + i + 3, indexOffset + i + 2
        });
    }

    indexOffset += modelVertexCount;
}

SavableChunk Chunk::makeSavableCopy() const {
    SavableChunk copy;

    copy.position = position;
    copy.blocks = blocks;
    if (!mesh.stagingVertices.empty()) {
        copy.vertices = std::move(mesh.stagingVertices);
    } else {
        copy.vertices = mesh.vertices;
    }
    if (!mesh.stagingIndices.empty()) {
        copy.indices = std::move(mesh.stagingIndices);
    } else {
        copy.indices = mesh.indices;
    }

    copy.hasMeshUpdate = mesh.hasNewMesh;

    return copy;
}
