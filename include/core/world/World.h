#ifndef WORLD_H
#define WORLD_H

#include <unordered_set>
#include <omp.h>

#include "core/world/Chunk.h"
#include "core/threads/ThreadSafeQueue.h"
#include "core/player/Player.h"

class World {
public:
    World();
    ~World();

    void init();

    void chunkHelperThread();
    void chunkWorkerThread();
    void meshWorkerThread();

    void generateMesh(const ChunkPosition& pos);

    void setBlockAtWorldPosition(int wx, int wy, int wz, int blockID);

    int sampleBlockID(int x, int y, int z) const;
    void markNeighborDirty(const ChunkPosition& pos, glm::ivec3 offset);

    void queueChunksForMeshing(const glm::vec3& playerPos);
    void updateChunksAroundPlayer(const glm::ivec3& playerChunk, const int VIEW_DISTANCE);
    std::vector<glm::ivec2> World::generateSpiralOffsets(int radius);

    void uploadChunkMeshes(int maxPerFrame = 2);
    void uploadMeshToGPU(Chunk& chunk);

    void queueChunksForRemoval(const glm::ivec3& centerChunk, const int VIEW_DISTANCE);
    void unloadDistantChunks();

    std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>, std::hash<ChunkPosition>> chunks;

private:
    std::thread chunkThread;
    std::thread meshThread;
    std::thread HchunkThread;

    std::atomic<bool> running = false;
    mutable std::mutex chunkMutex;

    ThreadSafeQueue<ChunkPosition> chunkCreationQueue;
    ThreadSafeQueue<std::shared_ptr<Chunk>> meshUploadQueue;
    ThreadSafeQueue<ChunkPosition> meshGenerationQueue;
    ThreadSafeQueue<std::shared_ptr<Chunk>> chunkRemovalQueue;

    std::unordered_set<ChunkPosition> queuedChunks;

    int minY = -2;
    int maxY = 10;

};

#endif
