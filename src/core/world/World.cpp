#include "core/world/World.h"

World::World() {}

World::~World() {
    running = false;

    chunkCreationQueue.stop();
    meshGenerationQueue.stop();
    meshUploadQueue.stop();
    chunkRemovalQueue.stop();
    chunkUploadQueue.stop();

    for (auto& thread : chunkGenThreads) if (thread.joinable()) thread.join();
    for (auto& thread : meshGenThreads) if (thread.joinable()) thread.join();
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
    
    for (int i = 0; i < (threads / 2) - 1; ++i)
        chunkGenThreads.emplace_back(&World::chunkWorkerThread, this);

    for (int i = 0; i < (threads / 2) - 1; ++i)
        meshGenThreads.emplace_back(&World::meshWorkerThread, this);

    chunkManagerThread = std::thread(&World::managerThread, this);
}

// Thread function for loading chunks around the player
void World::managerThread() {
    glm::ivec3 lastChunkPos = {INT_MAX, 0, INT_MAX};
    while (running) {
        auto current = Player::instance().getChunkPosition();
        if (current.x != lastChunkPos.x || current.z != lastChunkPos.z) {
            lastChunkPos = current;

            std::vector<ChunkPosition> drainedCreation;
            std::vector<std::shared_ptr<Chunk>> drainedMesh;

            chunkCreationQueue.drain(drainedCreation);
            meshGenerationQueue.drain(drainedMesh);

            for (const auto& pos : drainedCreation) chunkPositionSet.erase(pos);
            for (const auto& chunk : drainedMesh) chunkPositionSet.erase(chunk->getPosition());

            updateChunksAroundPlayer(current, Player::instance().VIEW_DISTANCE);
        }
        queueChunksForRemoval(current, Player::instance().VIEW_DISTANCE + 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Thread function for generating chunks around the player
void World::chunkWorkerThread() {
    constexpr int MIN_GENERATE_Y = 48;
    while (running) {
        ChunkPosition pos;
        if (!chunkCreationQueue.waitPop(pos)) return;

        if ((pos.y * CHUNK_SIZE + CHUNK_SIZE) < MIN_GENERATE_Y) continue;

        std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>();
        chunk->setPosition(pos);
        chunk->generateTerrain(0, 1, 0.5f, 2.0f, 0.008f, 3.5f);

        meshGenerationQueue.push(chunk);
    }
}

// Thread function for meshing chunks that have been marked for generation
void World::meshWorkerThread() {
    thread_local const auto& blockList = BlockRegister::instance().blocks;
    while (running) {
        std::shared_ptr<Chunk> chunk;

        if (!meshGenerationQueue.waitPop(chunk)) return;
        if (!chunk) continue;
        if (chunk->mesh.isEmpty) continue;
        if (std::abs(chunk->getPosition().x - Player::instance().getChunkPosition().x) > Player::instance().VIEW_DISTANCE ||
            std::abs(chunk->getPosition().z - Player::instance().getChunkPosition().z) > Player::instance().VIEW_DISTANCE) {
            continue;
        }
        generateMesh(chunk);
    }
}

// Generates the mesh for a chunk based on the task provided
void World::generateMesh(const std::shared_ptr<Chunk>& chunk) {
    
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    // auto chunkMapSnapshot = chunks;

    // std::array<std::shared_ptr<Chunk>, 27> neighborChunks;

    // for (int dy = -1; dy <= 1; ++dy) {
    //     for (int dz = -1; dz <= 1; ++dz) {
    //         for (int dx = -1; dx <= 1; ++dx) {
    //             ChunkPosition neighborPos = {
    //                 chunk->getPosition().x + dx,
    //                 chunk->getPosition().y + dy,
    //                 chunk->getPosition().z + dz
    //             };
    //             int idx = (dy + 1) * 9 + (dz + 1) * 3 + (dx + 1);
    //             auto it = chunkMapSnapshot.find(neighborPos);
    //             neighborChunks[idx] = (it != chunkMapSnapshot.end()) ? it->second : nullptr;
    //         }
    //     }
    // }

    chunk->generateMesh(vertices, indices,
        [this, chunk](glm::ivec3 offset, int x, int y, int z) -> int {
            int nx = x + offset.x;
            int ny = y + offset.y;
            int nz = z + offset.z;
        
            ChunkPosition relChunkOffset = {
                (nx < 0) ? -1 : (nx >= CHUNK_SIZE) ? 1 : 0,
                (ny < 0) ? -1 : (ny >= CHUNK_SIZE) ? 1 : 0,
                (nz < 0) ? -1 : (nz >= CHUNK_SIZE) ? 1 : 0
            };
        
            // int localX = (nx + CHUNK_SIZE) % CHUNK_SIZE;
            // int localY = (ny + CHUNK_SIZE) % CHUNK_SIZE;
            // int localZ = (nz + CHUNK_SIZE) % CHUNK_SIZE;
        
            // int neighborIdx = (relChunkOffset.y + 1) * 9 + (relChunkOffset.z + 1) * 3 + (relChunkOffset.x + 1);
            // auto& neighbor = neighborChunks[neighborIdx];
        
            // if (!neighbor) {
                int worldX = chunk->getPosition().x * CHUNK_SIZE + nx;
                int worldY = chunk->getPosition().y * CHUNK_SIZE + ny;
                int worldZ = chunk->getPosition().z * CHUNK_SIZE + nz;
        
                return Noise::getHeight(worldX, worldZ, 0, 1, 0.5f, 2.0f, 0.01f, 12.0f) > worldY ? 1 : 0;
            // }
        
            // return neighbor->getBlockID(localX, localY, localZ);
        });
         

    chunk->mesh.vertices = std::move(vertices);
    chunk->mesh.indices = std::move(indices);
    chunk->mesh.needsUpdate = false;
    chunk->mesh.isUploaded = false;
    chunk->mesh.isEmpty = chunk->mesh.vertices.empty() || chunk->mesh.indices.empty();

    if (!chunk->mesh.isEmpty) meshUploadQueue.push(chunk);
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
    auto sorted = generateSortedOffsets(VIEW_DISTANCE);

    for (const auto& offset : sorted) {
        for (int y = minY; y <= maxY; ++y) {
            ChunkPosition pos = {
                playerChunk.x + offset.x,
                y,
                playerChunk.z + offset.y
            };

            if (chunkPositionSet.find(pos) != chunkPositionSet.end()) continue;
            chunkPositionSet.insert(pos);
            chunkCreationQueue.push(pos);
        }
    }
}

std::vector<glm::ivec2> World::generateSortedOffsets(int radius) {
    std::vector<glm::ivec2> result;

    for (int dz = -radius; dz <= radius; ++dz) {
        for (int dx = -radius; dx <= radius; ++dx) {
            result.emplace_back(dx, dz);
        }
    }

    std::sort(result.begin(), result.end(), [](const glm::ivec2& a, const glm::ivec2& b) {
        return glm::length(glm::vec2(a)) < glm::length(glm::vec2(b));
    });

    return result;
}

// Uploads the chunk meshes to the GPU
void World::uploadChunkMeshes(int maxPerFrame) {
    for (int i = 0; i < maxPerFrame; ++i) {
        std::shared_ptr<Chunk> chunk;
        if (!meshUploadQueue.tryPop(chunk) || !chunk || chunk->mesh.isUploaded) continue;

        try {
            uploadMeshToGPU(*chunk);
            chunk->mesh.needsUpdate = false;
            chunk->mesh.isUploaded = true;
            chunkUploadQueue.push(chunk);
        } catch (...) {
            std::cerr << "Mesh upload error\n";
        }
    }
}

// Uploads the mesh data to the GPU
void World::uploadMeshToGPU(Chunk& chunk) {
    if (chunk.mesh.isUploaded || chunk.mesh.isEmpty) return;

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
            it = chunkPositionSet.erase(it);
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
