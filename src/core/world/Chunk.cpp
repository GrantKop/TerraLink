#include "core/world/Chunk.h"

Chunk::Chunk() {
    mesh.isEmpty = true;
}

Chunk::~Chunk() {}

// Generates terrain for the chunk by checking the biome map and using perlin noise
void Chunk::generateTerrain() {
    int worldMinY = position.y * CHUNK_SIZE;
    bool hasTerrain = false;

    int chunkWorldX = position.x * CHUNK_SIZE;
    int chunkWorldZ = position.z * CHUNK_SIZE;

    int heightMap[CHUNK_SIZE][CHUNK_SIZE];

    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            int worldX = chunkWorldX + x;
            int worldZ = chunkWorldZ + z;

            Biome blendedBiome = Noise::getBlendedBiome(worldX, worldZ);
            const BiomeTerrain& t = blendedBiome.terrain;

            float h = 0.0f;
            float freq = t.frequency;
            float amp = t.amplitude;

            for (int o = 0; o < t.octaves; ++o) {
                float sampleX = (worldX + t.noiseOffsetX) * freq;
                float sampleZ = (worldZ + t.noiseOffsetZ) * freq;
                h += snoise2(sampleX, sampleZ) * amp;

                freq *= t.lacunarity;
                amp *= t.persistence;
            }

            int height = static_cast<int>(t.baseHeight + h);
            heightMap[x][z] = height;

            if (height >= worldMinY) hasTerrain = true;
        }
    }

    if (!hasTerrain) return;

    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            int worldX = chunkWorldX + x;
            int worldZ = chunkWorldZ + z;

            Biome biome = Noise::getBlendedBiome(worldX, worldZ);
            int height = heightMap[x][z];

            for (int y = 0; y < CHUNK_SIZE; ++y) {
                int worldY = position.y * CHUNK_SIZE + y;
                if (worldY > height) continue;

                switch (biome.id) {
                    case BiomeID::_PLAINS:
                    case BiomeID::_FOREST:
                    case BiomeID::_HILLS:
                    case BiomeID::_TAIGA:
                    case BiomeID::_SAVANNA:
                        setBlockID(x, y, z, (worldY < height - 3) ? 1 : (worldY < height ? 3 : 2)); break;
                    case BiomeID::_DESERT:
                        setBlockID(x, y, z, (worldY < height - 3) ? 1 : 7); break;
                    case BiomeID::_MOUNTAINS:
                        if (worldY < height - 2) setBlockID(x, y, z, 1);
                        else if (worldY > 76) setBlockID(x, y, z, 6);
                        else setBlockID(x, y, z, 1); break;
                    case BiomeID::_RIVER:
                        if (worldY < height) setBlockID(x, y, z, 9); break;
                    default:
                        setBlockID(x, y, z, (worldY < height - 3) ? 1 : (worldY < height ? 3 : 2)); break;
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
        for (size_t i = 0; i < blockListPtr->size(); ++i)
            isTransparentCache[i] = (*blockListPtr)[i].isTransparent;
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

                // Handle model blocks (like covered_cross)
                if (block.model == "covered_cross") {
                    addCoveredCrossMesh(
                        block, x, y, z,
                        vertices, indices,
                        indexOffset,
                        chunkOffset
                    );
                    continue;
                }

                // Regular cube faces
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
                    if (!isTransparentCache[neighborID]) continue;

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
