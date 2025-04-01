#ifndef FLAT_WORLD_H
#define FLAT_WORLD_H

#include <unordered_set>
#include <omp.h>

#include "core/world/Chunk.h"
#include "core/threads/ThreadSafeQueue.h"
#include "core/player/Player.h"


struct ChunkMeshTask {
    ChunkPosition pos;
    float priority;

    bool operator<(const ChunkMeshTask& other) const {
        return priority < other.priority;
    }
};

class FlatWorld {
public:
    FlatWorld();
    ~FlatWorld();

    void chunkWorkerThread();
    void meshWorkerThread();

    void generateMesh(const ChunkMeshTask& task);

    void setBlockAtWorldPosition(int wx, int wy, int wz, int blockID);

    bool hasAllNeighbors(ChunkPosition pos) const;
    void markNeighborDirty(const ChunkPosition& pos, glm::ivec3 offset);
    void markNeighborsDirty(const ChunkPosition& pos);

    void queueChunksForMeshing(const glm::vec3& playerPos);
    void updateChunksAroundPlayer(const glm::ivec3& playerChunk, const int VIEW_DISTANCE);
    void uploadChunkMeshes(int maxPerFrame = 2);
    void uploadMeshToGPU(Chunk& chunk);

    void unloadDistantChunks(const glm::ivec3& centerChunk, const int VIEW_DISTANCE);

    std::unordered_map<ChunkPosition, Chunk, std::hash<ChunkPosition>> chunks;

    ThreadSafeQueue<ChunkPosition> chunkCreationQueue;
    ThreadSafeQueue<ChunkPosition> meshUploadQueue;
    ThreadSafeQueue<ChunkMeshTask> meshGenerationQueue;

private:
    std::thread chunkThread;
    std::thread meshThread;

    std::atomic<bool> running = true;
    mutable std::mutex chunkMutex;

};

#endif
