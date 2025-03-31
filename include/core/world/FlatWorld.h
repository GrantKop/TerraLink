#ifndef FLAT_WORLD_H
#define FLAT_WORLD_H

#include <queue>
#include <unordered_set>

#include "core/world/Chunk.h"

struct ChunkMeshTask {
    ChunkPosition pos;
    float priority;

    bool operator<(const ChunkMeshTask& other) const {
        return priority > other.priority;
    }
};
class FlatWorld {
public:
    void generateNecessaryChunkMeshes(int maxPerFrame = 2, const glm::vec3& playerPos = glm::vec3(0.0f, 0.0f, 0.0f));

    void setBlockAtWorldPosition(int wx, int wy, int wz, int blockID);

    bool hasAllNeighbors(ChunkPosition pos) const;
    void markNeighborDirty(const ChunkPosition& pos, glm::ivec3 offset);
    void markNeighborsDirty(const ChunkPosition& pos);

    void queueChunksForMeshing(const glm::vec3& playerPos);
    void updateChunksAroundPlayer(int centerX, int centerY, int centerZ, const int VIEW_DISTANCE);
    void uploadChunkMeshes(int maxPerFrame = 2);
    void uploadMeshToGPU(Chunk& chunk);

    void createChunks(int maxPerFrame = 2);
    void unloadDistantChunks(const glm::ivec3& centerChunk, const int VIEW_DISTANCE);

    std::unordered_map<ChunkPosition, Chunk, std::hash<ChunkPosition>> chunks;

    std::queue<ChunkPosition> chunkCreationQueue;

    std::priority_queue<ChunkMeshTask> meshGenerationQueue;
    std::queue<ChunkPosition> meshUploadQueue;

};

#endif
