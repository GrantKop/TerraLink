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

    void queueChunksForMeshing(const glm::vec3& playerPos);
    void updateChunksAroundPlayer(const glm::ivec3& playerChunk, const int VIEW_DISTANCE);
    std::vector<glm::ivec2> FlatWorld::generateSpiralOffsets(int radius);

    void uploadChunkMeshes(int maxPerFrame = 2);
    void uploadMeshToGPU(Chunk& chunk);

    void queueChunksForRemoval(const glm::ivec3& centerChunk, const int VIEW_DISTANCE);
    void unloadDistantChunks();

    std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>, std::hash<ChunkPosition>> chunks;

    ThreadSafeQueue<ChunkPosition> chunkCreationQueue;
    ThreadSafeQueue<std::shared_ptr<Chunk>> meshUploadQueue;
    ThreadSafeQueue<ChunkMeshTask> meshGenerationQueue;
    ThreadSafeQueue<std::shared_ptr<Chunk>> chunkRemovalQueue;

private:
    std::thread chunkThread;
    std::thread meshThread;
    // Thread vectors for chunk and mesh generation in potential future
    // std::vector<std::thread> meshThreads;
    // std::vector<std::thread> chunkThreads;

    std::atomic<bool> running = true;
    mutable std::mutex chunkMutex;

};

#endif
