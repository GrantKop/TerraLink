#include "core/world/World.h"

World* World::s_instance = nullptr;

// Sets the instance of the world
void World::setInstance(World* instance) {
    s_instance = instance;
}

// Returns the instance of the world
World& World::instance() {
    if (!s_instance) {
        s_instance = new World();
    }
    return *s_instance;
}

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
        if (chunk->mesh.isUploaded) {
            chunk->mesh.VAO.deleteBuffers();
            chunk->mesh.isUploaded = false;
        }
        chunk->mesh.vertices.clear();
        chunk->mesh.indices.clear();
    }

    chunks.clear();
}

// Initializes the world by starting the chunk generation and meshing threads
void World::init() {
    BiomeNoise::initializeNoiseGenerator();

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
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
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
        chunk->generateTerrain();

        meshGenerationQueue.push(chunk);
    }
}

// Thread function for meshing chunks that have been marked for generation
void World::meshWorkerThread() {
    thread_local const auto& blockList = BlockRegister::instance().blocks;
    while (running) {
        std::shared_ptr<Chunk> chunk;

        if (!meshUpdateQueue.tryPop(chunk)) {
            if (!meshGenerationQueue.tryPop(chunk)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
        }
        if (!chunk) continue;
        if (chunk->mesh.isEmpty) {
            chunkUploadQueue.push(chunk);
            continue;
        }
        if (!chunk->mesh.needsUpdate && chunk->mesh.isUploaded) continue;
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
    
    chunk->generateMesh(vertices, indices,
        [this, chunk](glm::ivec3 offset, int x, int y, int z) -> int {
            int nx = x + offset.x;
            int ny = y + offset.y;
            int nz = z + offset.z;
    
            int worldX = chunk->getPosition().x * CHUNK_SIZE + nx;
            int worldY = chunk->getPosition().y * CHUNK_SIZE + ny;
            int worldZ = chunk->getPosition().z * CHUNK_SIZE + nz;
    
            return 0; // Placeholder; you can restore noise-based fallback later
        });
    
    chunk->mesh.vertices = std::move(vertices);
    chunk->mesh.indices = std::move(indices);
    
    chunk->mesh.needsUpdate = false;
    chunk->mesh.isUploaded = false;
    chunk->mesh.isEmpty = chunk->mesh.vertices.empty() && chunk->mesh.indices.empty();

    if (!chunk->mesh.isEmpty) meshUploadQueue.push(chunk);
}

// Sets a block at the specified world position
void World::setBlockAtWorldPosition(int wx, int wy, int wz, int blockID) {
    ChunkPosition chunkPos = {
        (wx < 0 && wx % CHUNK_SIZE != 0) ? (wx / CHUNK_SIZE - 1) : (wx / CHUNK_SIZE),
        (wy < 0 && wy % CHUNK_SIZE != 0) ? (wy / CHUNK_SIZE - 1) : (wy / CHUNK_SIZE),
        (wz < 0 && wz % CHUNK_SIZE != 0) ? (wz / CHUNK_SIZE - 1) : (wz / CHUNK_SIZE)
    };    

    int localX = wx % CHUNK_SIZE;
    int localY = wy % CHUNK_SIZE;
    int localZ = wz % CHUNK_SIZE;

    if (localX < 0) localX += CHUNK_SIZE;
    if (localY < 0) localY += CHUNK_SIZE;
    if (localZ < 0) localZ += CHUNK_SIZE;

    std::shared_ptr<Chunk> chunk;

    auto it = chunks.find(chunkPos);
    if (it == chunks.end()) {
        std::shared_ptr<Chunk> newChunk = std::make_shared<Chunk>();
        newChunk->setPosition(chunkPos);
        chunk = newChunk;
    } else {
        chunk = it->second;
    }

    chunk->setBlockID(localX, localY, localZ, blockID);
    chunk->mesh.isEmpty = false;

    chunk->mesh.needsUpdate = true;
    meshUpdateQueue.push(chunk); 

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

    if (!chunk.mesh.vertices.empty()) {
        chunk.mesh.VAO.init();
        chunk.mesh.VAO.bind();
        chunk.mesh.VAO.addVertexBuffer(chunk.mesh.vertices);
        chunk.mesh.VAO.addElementBuffer(chunk.mesh.indices);
        chunk.mesh.VAO.addAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        chunk.mesh.VAO.addAttribute(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        chunk.mesh.VAO.addAttribute(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    }
}

// Uploads the chunk meshes to the map
void World::uploadChunksToMap() {
    int uploadedChunks = 0;
    std::shared_ptr<Chunk> chunk;

    while (uploadedChunks < 5) {
        if (chunkUploadQueue.tryPop(chunk)) {
            if (!chunk) continue;

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

    if (std::abs(pos.x - Player::instance().getChunkPosition().x) <= Player::instance().VIEW_DISTANCE &&
        std::abs(pos.z - Player::instance().getChunkPosition().z) <= Player::instance().VIEW_DISTANCE) {
        return;
    }

    auto it = chunks.find(pos);
    if (it == chunks.end()) return;

    std::shared_ptr<Chunk> chunkPtr = it->second;
    if (!chunkPtr) return;
    if (chunkPtr->mesh.isUploaded) {
        try {
            chunkPtr->mesh.VAO.deleteBuffers();
        } catch (...) {
            std::cerr << "Exception in deleteBuffers!" << std::endl;
        }
        chunkPtr->mesh.vertices.clear();
        chunkPtr->mesh.indices.clear();
    }

    chunks.erase(it);
}

int World::getBlockIDAtWorldPosition(int wx, int wy, int wz) const {
    ChunkPosition chunkPos = {
        (wx < 0 && wx % CHUNK_SIZE != 0) ? (wx / CHUNK_SIZE - 1) : (wx / CHUNK_SIZE),
        (wy < 0 && wy % CHUNK_SIZE != 0) ? (wy / CHUNK_SIZE - 1) : (wy / CHUNK_SIZE),
        (wz < 0 && wz % CHUNK_SIZE != 0) ? (wz / CHUNK_SIZE - 1) : (wz / CHUNK_SIZE)
    };    

    int localX = wx % CHUNK_SIZE;
    int localY = wy % CHUNK_SIZE;
    int localZ = wz % CHUNK_SIZE;

    if (localX < 0) localX += CHUNK_SIZE;
    if (localY < 0) localY += CHUNK_SIZE;
    if (localZ < 0) localZ += CHUNK_SIZE;

    auto it = chunks.find(chunkPos);
    if (it == chunks.end() || !it->second) return 0;

    return it->second->getBlockID(localX, localY, localZ);
}
