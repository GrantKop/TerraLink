#include "core/world/World.h"

World::World() {}

World::~World() {
    running = false;

    chunkCreationQueue.stop();
    meshGenerationQueue.stop();
    meshUploadQueue.stop();
    chunkRemovalQueue.stop();
    chunkUploadQueue.stop();

    for (auto& thread : chunkGenThreads) {
        if (thread.joinable()) thread.join();
    }
    for (auto& thread : meshGenThreads) {
        if (thread.joinable()) thread.join();
    }
    if (chunkManagerThread.joinable()) chunkManagerThread.join();
    
    for (auto& [pos, chunk] : chunks) {
        if (chunk && chunk->mesh.isUploaded) {
            chunk->mesh.VAO.deleteBuffers();
            chunk->mesh.vaoInitialized = false;
            chunk->mesh.vertices.clear();
            chunk->mesh.indices.clear();
        }
    }    

    chunks.clear();
}

// Initializes the world by starting the chunk generation and meshing threads
void World::init() {
    if (running) return;
    running = true;

    int threads = std::max(1u, std::thread::hardware_concurrency());
    
    for (int i = 0; i < threads / 2; ++i)
        chunkGenThreads.emplace_back(&World::chunkWorkerThread, this);

    for (int i = 0; i < threads / 2; ++i)
        meshGenThreads.emplace_back(&World::meshWorkerThread, this);

    chunkManagerThread = std::thread(&World::managerThread, this);
}

// Thread function for loading chunks around the player
void World::managerThread() {
    glm::ivec3 lastChunkPos = {INT_MAX, INT_MAX, INT_MAX};
    while (running) {
        if (Player::instance().getChunkPosition().x != lastChunkPos.x || Player::instance().getChunkPosition().z != lastChunkPos.z) {
            lastChunkPos = Player::instance().getChunkPosition();

            std::vector<ChunkPosition> clearedChunkPositions;
            std::vector<std::shared_ptr<Chunk>> clearedMeshTasks;

            chunkCreationQueue.drain(clearedChunkPositions);
            meshGenerationQueue.drain(clearedMeshTasks);

            for (const auto& pos : clearedChunkPositions) {
                chunkPositionSet.erase(pos);
            }
            for (const auto& chunk : clearedMeshTasks) {
                chunkPositionSet.erase(chunk->getPosition());
            }

            updateChunksAroundPlayer(lastChunkPos, Player::instance().VIEW_DISTANCE);
            queueChunksForRemoval(lastChunkPos, Player::instance().VIEW_DISTANCE + 1);
        } 
    }
}

// Thread function for generating chunks around the player
void World::chunkWorkerThread() {
    constexpr int MIN_GENERATE_Y = 48;
    while (running) {
        ChunkPosition pos;
        if (!chunkCreationQueue.waitPop(pos)) return;

        int chunkTopY = pos.y * CHUNK_SIZE + CHUNK_SIZE;
        if (chunkTopY < MIN_GENERATE_Y) continue;

        std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>();
        chunk->setPosition(pos);
        chunk->generateTerrain(0, 1, 0.5f, 2.0f, 0.008f, 3.5f);

        meshGenerationQueue.push(chunk);
    }
}

// Thread function for meshing chunks that have been marked for generation
void World::meshWorkerThread() {
    while (running) {
        std::shared_ptr<Chunk> chunk;

        if (!meshGenerationQueue.waitPop(chunk)) return;
        if (!chunk) continue;
        if (glm::distance((float)Player::instance().getChunkPosition().x, (float)chunk->getPosition().x) > Player::instance().VIEW_DISTANCE 
        ||  glm::distance((float)Player::instance().getChunkPosition().z, (float)chunk->getPosition().z) > Player::instance().VIEW_DISTANCE) {
            continue;
        }
        generateMesh(chunk);
    }
}

// Generates the mesh for a chunk based on the task provided
void World::generateMesh(const std::shared_ptr<Chunk>& chunk) {

    if (!chunk) return;

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    chunk->generateMesh(vertices, indices,
    [this, chunk](glm::ivec3 offset, int x, int y, int z) -> int {
        glm::ivec3 localCoord = glm::ivec3(x, y, z) + offset;
        ChunkPosition targetChunkPos = chunk->getPosition();

        if (localCoord.x < 0) { localCoord.x += CHUNK_SIZE; targetChunkPos.x -= 1; }
        else if (localCoord.x >= CHUNK_SIZE) { localCoord.x -= CHUNK_SIZE; targetChunkPos.x += 1; }

        if (localCoord.y < 0) { localCoord.y += CHUNK_SIZE; targetChunkPos.y -= 1; }
        else if (localCoord.y >= CHUNK_SIZE) { localCoord.y -= CHUNK_SIZE; targetChunkPos.y += 1; }

        if (localCoord.z < 0) { localCoord.z += CHUNK_SIZE; targetChunkPos.z -= 1; }
        else if (localCoord.z >= CHUNK_SIZE) { localCoord.z -= CHUNK_SIZE; targetChunkPos.z += 1; }

        // {
        //     std::lock_guard<std::mutex> chunkLock(chunkMutex);
        //     auto it = chunks.find(targetChunkPos);
        //     if (it != chunks.end() && it->second) {
        //         return it->second->getBlockID(localCoord.x, localCoord.y, localCoord.z);
        //     }
        // }
        return 0;
        int worldX = localCoord.x + targetChunkPos.x * CHUNK_SIZE;
        int worldY = localCoord.y + targetChunkPos.y * CHUNK_SIZE;
        int worldZ = localCoord.z + targetChunkPos.z * CHUNK_SIZE;

        return sampleBlockID(worldX, worldY, worldZ);
    });

    chunk->mesh.vertices = std::move(vertices);
    chunk->mesh.indices = std::move(indices);
    chunk->mesh.needsUpdate = false;
    chunk->mesh.isUploaded = false;
    chunk->mesh.isEmpty = chunk->mesh.vertices.empty() || chunk->mesh.indices.empty();

    if (chunk->mesh.isEmpty) return;

    meshUploadQueue.push(chunk);
}

// Sets a block at the specified world position
void World::setBlockAtWorldPosition(int wx, int wy, int wz, int blockID) {
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

    std::shared_ptr<Chunk> chunk = it->second;
    chunk->setBlockID(localX, localY, localZ, blockID);
    chunk->mesh.needsUpdate = true;

    if (localX == 0) markNeighborDirty(chunkPos, { -1, 0, 0 });
    if (localX == CHUNK_SIZE - 1) markNeighborDirty(chunkPos, { 1, 0, 0 });

    if (localY == 0) markNeighborDirty(chunkPos, { 0, -1, 0 });
    if (localY == CHUNK_SIZE - 1) markNeighborDirty(chunkPos, { 0, 1, 0 });

    if (localZ == 0) markNeighborDirty(chunkPos, { 0, 0, -1 });
    if (localZ == CHUNK_SIZE - 1) markNeighborDirty(chunkPos, { 0, 0, 1 });
}

// Samples a block ID at the specified world position
int World::sampleBlockID(int wx, int wy, int wz) const {
    
    float height = Noise::getHeight(wx, wz, 0, 1, 0.5f, 2.0f, 0.01f, 16.0f);
    return wy < height ? BlockRegister::instance().getBlockByIndex(1).ID
                      : BlockRegister::instance().getBlockByIndex(0).ID;
}

// Marks a specific neighboring chunk as dirty, indicating that it needs to be updated
// WIP
void World::markNeighborDirty(const ChunkPosition& pos, glm::ivec3 offset) {
    ChunkPosition neighborPos = {
        pos.x + offset.x,
        pos.y + offset.y,
        pos.z + offset.z
    };
}

// Updates the chunks around the player based on their position
void World::updateChunksAroundPlayer(const glm::ivec3& playerChunk, const int VIEW_DISTANCE) {
    auto spiral = generateSpiralOffsets(VIEW_DISTANCE);
    int queuedThisFrame = 0;
    int maxQueuePerFrame = 128;

    for (const auto& offset : spiral) {
        for (int y = minY; y <= maxY; ++y) {
            //if (queuedThisFrame >= maxQueuePerFrame) return;
            ChunkPosition pos = {
                playerChunk.x + offset.x,
                y,
                playerChunk.z + offset.y
            };

            if (chunkPositionSet.find(pos) != chunkPositionSet.end()) continue;
            chunkPositionSet.insert(pos);
            chunkCreationQueue.push(pos);
           //++queuedThisFrame;
        }
    }
}

// Generates a spiral pattern of offsets for chunk generation
std::vector<glm::ivec2> World::generateSpiralOffsets(int radius) {
    std::vector<glm::ivec2> result;

    int x = 0, z = 0;
    int dx = 0, dz = -1;

    int sideLength = radius * 2;
    int maxSteps = sideLength * sideLength;

    for (int i = 0; i < maxSteps; ++i) {
        if (std::abs(x) <= radius && std::abs(z) <= radius) {
            result.push_back({x, z});
        }

        if (x == z || (x < 0 && x == -z) || (x > 0 && x == 1 - z)) {
            int temp = dx;
            dx = -dz;
            dz = temp;
        }

        x += dx;
        z += dz;
    }

    return result;
}

// Uploads the chunk meshes to the GPU
void World::uploadChunkMeshes(int maxPerFrame) {
    int uploadedChunks = 0;
    while (uploadedChunks < maxPerFrame) {
        std::shared_ptr<Chunk> chunkPtr = nullptr;
        if (!meshUploadQueue.tryPop(chunkPtr)) break;
        if (!chunkPtr) continue;

        if (chunkPtr->mesh.vertices.empty() || chunkPtr->mesh.indices.empty() || chunkPtr->mesh.isUploaded) continue;

        try {
            uploadMeshToGPU(*chunkPtr);
            chunkPtr->mesh.needsUpdate = false;
            chunkPtr->mesh.isUploaded = true;
        } catch (const std::exception& e) {
            std::cerr << "Caught exception during mesh upload: " << e.what() << std::endl;
            continue;
        } catch (...) {
            std::cerr << "Caught unknown exception during mesh upload." << std::endl;
            continue;
        }
        uploadedChunks++;
        chunkUploadQueue.push(chunkPtr);
    }
}

// Uploads the mesh data to the GPU
void World::uploadMeshToGPU(Chunk& chunk) {
    if (chunk.mesh.isUploaded) return;
    assert(!chunk.mesh.vertices.empty());
    assert(!chunk.mesh.indices.empty());

    if (!chunk.mesh.vaoInitialized) {
        chunk.mesh.VAO.init();
        chunk.mesh.vaoInitialized = true;
    }

    chunk.mesh.VAO.bind();
    chunk.mesh.VAO.addVertexBuffer(chunk.mesh.vertices);
    chunk.mesh.VAO.addElementBuffer(chunk.mesh.indices);
    chunk.mesh.VAO.addAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    chunk.mesh.VAO.addAttribute(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    chunk.mesh.VAO.addAttribute(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
}

// Uploads the chunk meshes to the map
void World::uploadChunksToMap() {
    int uploadedChunks = 0;

    while (uploadedChunks < 2) {
        std::shared_ptr<Chunk> chunk;
        if (chunkUploadQueue.tryPop(chunk)) {
            if (!chunk) continue;

            auto it = chunks.find(chunk->getPosition());
            if (it != chunks.end()) continue;
            chunks[chunk->getPosition()] = chunk;
            uploadedChunks++;
        } else {
            return;
        }
    }
}

// Unloads distant chunks based on the player's position and view distance
void World::queueChunksForRemoval(const glm::ivec3& centerChunk, const int VIEW_DISTANCE) {

    int maxX = centerChunk.x + VIEW_DISTANCE;
    int maxZ = centerChunk.z + VIEW_DISTANCE;
    int minX = centerChunk.x - VIEW_DISTANCE;
    int minZ = centerChunk.z - VIEW_DISTANCE;

    for (auto it = chunkPositionSet.begin(); it != chunkPositionSet.end(); ) {
        const ChunkPosition& pos = *it;
        if (pos.x > maxX || pos.x < minX || pos.z > maxZ || pos.z < minZ) {
            chunkRemovalQueue.push(pos);
            it = chunkPositionSet.erase(it); // safely erase and update iterator
        } else {
            ++it;
        }
    }
}

// Unloads distant chunks that are no longer needed
void World::unloadDistantChunks() {
    ChunkPosition pos;
    if (!chunkRemovalQueue.tryPop(pos)) return;

    auto it = chunks.find(pos);
    if (it == chunks.end()) return;

    std::shared_ptr<Chunk> chunkPtr = it->second;
    if (!chunkPtr) return;
    if (chunkPtr->mesh.isUploaded && chunkPtr->mesh.vaoInitialized) {
        try {
            chunkPtr->mesh.VAO.deleteBuffers();
        } catch (...) {
            std::cerr << "Exception in deleteBuffers!" << std::endl;
        }
        chunkPtr->mesh.vaoInitialized = false;
        chunkPtr->mesh.vertices.clear();
        chunkPtr->mesh.indices.clear();
    }

    chunks.erase(it);
}
