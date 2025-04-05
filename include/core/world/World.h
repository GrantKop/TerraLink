#ifndef WORLD_H
#define WORLD_H

#include <unordered_set>

#include "core/world/Chunk.h"
#include "core/player/Player.h"

class World {
public:
    World();
    ~World();

    void init();

    void managerThread();
    void chunkWorkerThread();
    void meshWorkerThread();

    void generateMesh(const std::shared_ptr<Chunk>& chunk);

    void setBlockAtWorldPosition(int wx, int wy, int wz, int blockID);

    int sampleBlockID(int x, int y, int z) const;
    void markNeighborDirty(const ChunkPosition& pos, glm::ivec3 offset);

    void queueChunksForMeshing(const glm::vec3& playerPos);
    void updateChunksAroundPlayer(const glm::ivec3& playerChunk, const int VIEW_DISTANCE);
    std::vector<glm::ivec2> generateSortedOffsets(int radius);

    void uploadChunkMeshes(int maxPerFrame = 2);
    void uploadMeshToGPU(Chunk& chunk);

    void uploadChunksToMap();

    void queueChunksForRemoval(const glm::ivec3& centerChunk, const int VIEW_DISTANCE);
    void unloadDistantChunks();

    std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>, std::hash<ChunkPosition>> chunks;

private:
    std::vector<std::thread> chunkGenThreads;
    std::vector<std::thread> meshGenThreads;
    std::thread chunkManagerThread;

    std::atomic<bool> running = false;
    
    ThreadSafeQueue<ChunkPosition> chunkCreationQueue;
    ThreadSafeQueue<std::shared_ptr<Chunk>> meshGenerationQueue;
    ThreadSafeQueue<std::shared_ptr<Chunk>> chunkUploadQueue;
    ThreadSafeQueue<std::shared_ptr<Chunk>> meshUploadQueue;
    ThreadSafeQueue<ChunkPosition> chunkRemovalQueue;

    std::unordered_set<ChunkPosition> chunkPositionSet;

    int minY = -2;
    int maxY = 10;

};

#endif
