#ifndef WORLD_H
#define WORLD_H

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#ifdef T
#undef T
#endif

#define NOMINMAX

#include <unordered_set>
#include <chrono>
#include <ctime>
#include <zstd.h>
#include <zstd_errors.h>

#include "core/world/Chunk.h"
#include "core/world/Cloud.h"
#include "graphics/Shader.h"
#include "network/TCPSocket.h"

class Player;

class World {
public:
    explicit World(const std:: string& saveDir = "saves/");
    ~World();

    void shutdown();

    void init();

    static void setInstance(World* instance);
    static World& instance();

    void managerThread();
    void chunkWorkerThread();
    void meshWorkerThread();

    void chunkUpdateThread();

    void networkWorker(ChunkPosition pos);
    bool requestChunkOverUDP(const ChunkPosition& pos, std::shared_ptr<Chunk>& outChunk);
    void sendChunkOverUDP(SavableChunk chunk);

    void sendChunkUpdate(SavableChunk chunk);
    void pollTCPMessages();

    void generateMesh(const std::shared_ptr<Chunk>& chunk);

    void setBlockAtWorldPosition(int wx, int wy, int wz, int blockID);

    void queueChunksForMeshing(const glm::vec3& playerPos);
    void updateChunksAroundPlayer(const glm::ivec3& playerChunk, const int VIEW_DISTANCE);
    std::vector<glm::ivec2> generateSortedOffsets(int radius);

    void uploadChunkMeshes(int maxPerFrame = 2);
    void uploadMeshToGPU(Chunk& chunk);

    void uploadChunksToMap();

    void queueChunksForRemoval(const glm::ivec3& centerChunk, const int VIEW_DISTANCE);
    void unloadDistantChunks();

    int getBlockIDAtWorldPosition(int wx, int wy, int wz) const;

    bool collidesWithBlockAABB(glm::vec3 position, glm::vec3 size) const;
    bool wouldBlockOverlapPlayer(const glm::ivec3& blockPos) const;

    void saveChunkToFile(const std::shared_ptr<Chunk>& chunk);
    bool loadChunkFromFile(const ChunkPosition& pos, std::shared_ptr<Chunk>& chunkOut);

    void setSaveDirectory(const std::string& saveDir);
    void createSaveDirectory();
    void createWorldConfigFile();

    void savePlayerData(Player& player, const std::string& playerID);
    bool loadPlayerData(Player& player, const std::string& playerID);
    std::string getPlayerID() const;

    void chunkReset();

    void serializeChunk(SavableChunk& chunk, std::vector<uint8_t>& out);
    std::shared_ptr<Chunk> deserializeChunk(const std::vector<uint8_t>& in);

    void setSeed(uint32_t newSeed) { seed = newSeed; }
    uint32_t getSeed() const { return seed; }

    std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>, std::hash<ChunkPosition>> chunks;
    std::unordered_set<ChunkPosition> chunkPositionSet;

    std::unordered_map<CloudPosition, CloudMesh> clouds;
    std::unordered_set<CloudPosition> activeClouds;

    std::atomic<bool> needsFullReset = false;

    // Clouds
    std::vector<Vertex> cloudVertices;
    std::vector<GLuint> cloudIndices;
    VertexArrayObject cloudVAO;
    bool cloudsUploaded = false;

    void updateCloudsAroundPlayer();
    void drawClouds(Shader& cloudShader);

private:
    std::vector<std::thread> chunkGenThreads;
    std::vector<std::thread> meshGenThreads;
    std::thread chunkManagerThread;
    std::thread networkThread;

    std::atomic<bool> running = false;
    std::atomic<bool> udpReceiving = false;
    
    ThreadSafeQueue<ChunkPosition> chunkCreationQueue;
    ThreadSafeQueue<std::shared_ptr<Chunk>> meshGenerationQueue;
    ThreadSafeQueue<std::shared_ptr<Chunk>> chunkUploadQueue;
    ThreadSafeQueue<std::shared_ptr<Chunk>> meshUploadQueue;
    ThreadSafeQueue<ChunkPosition> chunkRemovalQueue;
    ThreadSafeQueue<SavableChunk> chunkSaveQueue;

    ThreadSafeQueue<std::shared_ptr<Chunk>> meshUpdateQueue;

    SOCKET tcpSocket;

    uint32_t seed = 1;

    int minY = -5;
    int maxY = 15;

    std::string saveDirectory;

    static World* s_instance;

};

#endif
