#include "core/world/FlatWorld.h"

void FlatWorld::generateNecessaryChunkMeshes(int maxPerFrame, const glm::vec3& playerPos) {

    int processedChunks = 0;

    while (!meshGenerationQueue.empty() && processedChunks < maxPerFrame) {
        ChunkMeshTask task = meshGenerationQueue.top();
        meshGenerationQueue.pop();

        auto it = chunks.find(task.pos);
        if (it == chunks.end()) continue;

        Chunk& chunk = it->second;

        if (!chunk.mesh.needsUpdate) continue;

        chunk.generateMesh(chunk.mesh.vertices, chunk.mesh.indices,
            [xOffset = task.pos, this](glm::ivec3 offset, int x, int y, int z) -> int {
                glm::ivec3 neighborChunkOffset = offset;
                glm::ivec3 localCoord = glm::ivec3(x, y, z) + offset;
            
                ChunkPosition targetChunkPos = xOffset;
                if (localCoord.x < 0) { localCoord.x += CHUNK_SIZE; targetChunkPos.x -= 1; }
                else if (localCoord.x >= CHUNK_SIZE) { localCoord.x -= CHUNK_SIZE; targetChunkPos.x += 1; }
            
                if (localCoord.y < 0) { localCoord.y += CHUNK_SIZE; targetChunkPos.y -= 1; }
                else if (localCoord.y >= CHUNK_SIZE) { localCoord.y -= CHUNK_SIZE; targetChunkPos.y += 1; }
            
                if (localCoord.z < 0) { localCoord.z += CHUNK_SIZE; targetChunkPos.z -= 1; }
                else if (localCoord.z >= CHUNK_SIZE) { localCoord.z -= CHUNK_SIZE; targetChunkPos.z += 1; }
            
                auto it = chunks.find(targetChunkPos);
                if (it != chunks.end()) {
                    return it->second.getBlockID(localCoord.x, localCoord.y, localCoord.z);
                }
            
                return BlockRegister::instance().getBlockByIndex(0).ID;
            });
        
        chunk.mesh.needsUpdate = false;
        chunk.mesh.isUploaded = false;
        
        meshUploadQueue.push(task.pos);
        processedChunks++;
    }
}

// Sets a block at the specified world position
void FlatWorld::setBlockAtWorldPosition(int wx, int wy, int wz, int blockID) {
    ChunkPosition chunkPos = {
        wx / CHUNK_SIZE,
        wy / CHUNK_SIZE,
        wz / CHUNK_SIZE
    };

    int localX = wx % CHUNK_SIZE;
    int localY = wy % CHUNK_SIZE;
    int localZ = wz % CHUNK_SIZE;

    if (localX < 0) localX += CHUNK_SIZE;
    if (localY < 0) localY += CHUNK_SIZE;
    if (localZ < 0) localZ += CHUNK_SIZE;

    auto it = chunks.find(chunkPos);
    if (it == chunks.end()) return;

    Chunk& chunk = it->second;
    chunk.setBlockID(localX, localY, localZ, blockID);
    chunk.mesh.needsUpdate = true;

    if (localX == 0) markNeighborDirty(chunkPos, { -1, 0, 0 });
    if (localX == CHUNK_SIZE - 1) markNeighborDirty(chunkPos, { 1, 0, 0 });

    if (localY == 0) markNeighborDirty(chunkPos, { 0, -1, 0 });
    if (localY == CHUNK_SIZE - 1) markNeighborDirty(chunkPos, { 0, 1, 0 });

    if (localZ == 0) markNeighborDirty(chunkPos, { 0, 0, -1 });
    if (localZ == CHUNK_SIZE - 1) markNeighborDirty(chunkPos, { 0, 0, 1 });
}

// Checks if all neighboring chunks are present and loaded
bool FlatWorld::hasAllNeighbors(ChunkPosition pos) const {
    for (const glm::ivec3& dir : {
        glm::ivec3(-1, 0, 0), glm::ivec3(1, 0, 0),
        glm::ivec3(0, 0, -1), glm::ivec3(0, 0, 1),
        glm::ivec3(0, -1, 0), glm::ivec3(0, 1, 0)
    }) {
        ChunkPosition neighborPos = {
            pos.x + dir.x,
            0,
            pos.z + dir.z
        };

        if (chunks.find(neighborPos) == chunks.end())
            return false;
    }

    return true;
}

// Marks a specific neighboring chunk as dirty, indicating that it needs to be updated
void FlatWorld::markNeighborDirty(const ChunkPosition& pos, glm::ivec3 offset) {
    ChunkPosition neighborPos = {
        pos.x + offset.x,
        pos.y + offset.y,
        pos.z + offset.z
    };

    auto it = chunks.find(neighborPos);
    if (it != chunks.end()) {
        it->second.mesh.needsUpdate = true;
    }
}

// Marks the neighboring chunks as dirty, indicating that they need to be updated
void FlatWorld::markNeighborsDirty(const ChunkPosition& pos) {
    static const std::vector<glm::ivec3> directions = {
        { 1, 0, 0 }, {-1, 0, 0},
        { 0, 0, 1 }, { 0, 0, -1 },
        { 0, 1, 0 }, { 0, -1, 0 }
    };

    for (const auto& dir : directions) {
        ChunkPosition neighborPos = {
            pos.x + dir.x,
            pos.y + dir.y,
            pos.z + dir.z
        };

        auto it = chunks.find(neighborPos);
        if (it != chunks.end()) {
            it->second.mesh.needsUpdate = true;
        }
    }
}

// Queues chunks for meshing based on the player's position
void FlatWorld::queueChunksForMeshing(const glm::vec3& playerPos) {
    for (auto& [pos, chunk] : chunks) {
        if (chunk.mesh.needsUpdate && hasAllNeighbors(pos)) {
            glm::vec3 chunkCenter = glm::vec3(pos.x, pos.y, pos.z) * (float)CHUNK_SIZE;
            float distance = glm::distance(playerPos, chunkCenter);

            meshGenerationQueue.push({pos, distance});
        }
    }
}

// Updates the chunks around the player based on their position
void FlatWorld::updateChunksAroundPlayer(int centerX, int centerY, int centerZ, const int VIEW_DISTANCE) {
    const int radius = VIEW_DISTANCE;

    for (int x = -radius; x <= radius; ++x) {
        for (int z = -radius; z <= radius; ++z) {
            ChunkPosition pos = { centerX + x, 0, centerZ + z };
            if (chunks.find(pos) == chunks.end()) {
                chunkCreationQueue.push(pos);
            }
        }
    }
}

// Uploads the chunk meshes to the GPU
void FlatWorld::uploadChunkMeshes(int maxPerFrame) {
    int processedChunks = 0;

    while (!meshUploadQueue.empty() && processedChunks < maxPerFrame) {
        ChunkPosition pos = meshUploadQueue.front();
        meshUploadQueue.pop();

        auto it = chunks.find(pos);
        if (it == chunks.end()) continue;

        uploadMeshToGPU(it->second);
        processedChunks++;
    }
}

// Uploads the mesh data to the GPU
void FlatWorld::uploadMeshToGPU(Chunk& chunk) {
    if (chunk.mesh.isUploaded || chunk.mesh.vertices.empty() || chunk.mesh.indices.empty()) return;

    chunk.mesh.VAO.bind();
    chunk.mesh.VAO.addVertexBuffer(chunk.mesh.vertices);
    chunk.mesh.VAO.addElementBuffer(chunk.mesh.indices);
    chunk.mesh.VAO.addAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    chunk.mesh.VAO.addAttribute(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    chunk.mesh.VAO.addAttribute(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

    chunk.mesh.isUploaded = true;
}

// Creates chunks based on the queued positions
void FlatWorld::createChunks(int maxPerFrame) {
    int createdChunks = 0;

    while (!chunkCreationQueue.empty() && createdChunks < maxPerFrame) {
        ChunkPosition pos = chunkCreationQueue.front();
        chunkCreationQueue.pop();

        if (chunks.find(pos) != chunks.end()) continue;

        Chunk chunk;
        chunk.setPosition(pos);
        chunks[pos] = chunk;
        markNeighborsDirty(pos);

        createdChunks++;
    }
}

// Unloads distant chunks based on the player's position and view distance
void FlatWorld::unloadDistantChunks(const glm::ivec3& centerChunk, const int VIEW_DISTANCE) {
    std::vector<ChunkPosition> chunksToUnload;

    for (auto& [pos, chunk] : chunks) {
        int dx = pos.x - centerChunk.x;
        int dy = pos.y - centerChunk.y;
        int dz = pos.z - centerChunk.z;

        if (abs(dx) > VIEW_DISTANCE || abs(dy) > VIEW_DISTANCE || abs(dz) > VIEW_DISTANCE) {
            chunksToUnload.push_back(pos);
        }
    }

    for (const auto& pos : chunksToUnload) {
        auto it = chunks.find(pos);
        if (it != chunks.end()) {
            if (it->second.mesh.isUploaded) {
                it->second.mesh.VAO.deleteBuffers();
            }

            chunks.erase(it);
        }
    }
}
