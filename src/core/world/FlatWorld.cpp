#include "core/world/FlatWorld.h"

FlatWorld::FlatWorld() {
    int maxThreads = omp_get_max_threads();

    chunkThread = std::thread(&FlatWorld::chunkWorkerThread, this);
    meshThread = std::thread(&FlatWorld::meshWorkerThread, this);
}

FlatWorld::~FlatWorld() {
    running = false;

    chunkCreationQueue.stop();
    meshGenerationQueue.stop();
    meshUploadQueue.stop();
    chunkRemovalQueue.stop();

    if (chunkThread.joinable()) chunkThread.join();
    if (meshThread.joinable()) meshThread.join();

}

// Thread function for generating chunks around the player
void FlatWorld::chunkWorkerThread() {
    glm::ivec3 lastChunkPos = {INT_MAX, INT_MAX, INT_MAX};
    while (running) {
        if (Player::instance().getChunkPosition() != lastChunkPos) {
            std::lock_guard<std::mutex> lock(chunkMutex);
            lastChunkPos = Player::instance().getChunkPosition();
            updateChunksAroundPlayer(lastChunkPos, Player::instance().VIEW_DISTANCE);
            queueChunksForRemoval(lastChunkPos, Player::instance().VIEW_DISTANCE + 1);
        }

        ChunkPosition pos;
        if (chunkCreationQueue.tryPop(pos)) {
        {
            std::lock_guard<std::mutex> lock(this->chunkMutex);
            if (chunks.find(pos) == chunks.end()) {

                std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>();
                chunk->setPosition(pos);
                if (glm::distance(glm::vec3{Player::instance().getChunkPosition()}, {pos.x, pos.y, pos.z}) > Player::instance().VIEW_DISTANCE) {
                    chunk->mesh.shouldRender = false;
                }
                chunks[pos] = std::move(chunk);
            }
        }

        if (!chunks[pos]->mesh.shouldRender) continue;

        glm::vec3 chunkCenter = glm::vec3(pos.x, pos.y, pos.z) * (float)CHUNK_SIZE;
        float distance = glm::distance(Player::instance().getPosition(), chunkCenter);
        meshGenerationQueue.push({pos, distance});

        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

// Thread function for generating chunk meshes
void FlatWorld::meshWorkerThread() {
    while (running) {
        ChunkMeshTask task;
        if (!meshGenerationQueue.waitPop(task)) return;
        generateMesh(task);
    }
}

// Generates the mesh for a chunk based on the task provided
void FlatWorld::generateMesh(const ChunkMeshTask& task) {
    std::lock_guard<std::mutex> lock(chunkMutex);
    auto it = chunks.find(task.pos);
    if (it == chunks.end()) return;

    if (!hasAllNeighbors(task.pos)) {
        meshGenerationQueue.push(task);
        return;
    }

    std::shared_ptr<Chunk> chunk = it->second;
    if (!chunk->mesh.needsUpdate) return;

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    chunk->generateMesh(vertices, indices,
        [xOffset = task.pos, this](glm::ivec3 offset, int x, int y, int z) -> int {
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
                return it->second->getBlockID(localCoord.x, localCoord.y, localCoord.z);
            }

            return BlockRegister::instance().getBlockByIndex(0).ID;
        });

    chunk->mesh.vertices = std::move(vertices);
    chunk->mesh.indices = std::move(indices);
    chunk->mesh.needsUpdate = false;
    chunk->mesh.isUploaded = false;
    chunk->mesh.markedForUpload = true;

    meshUploadQueue.push(chunks[task.pos]);
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

// Checks if all neighboring chunks are present and loaded
bool FlatWorld::hasAllNeighbors(ChunkPosition pos) const {
    for (const glm::ivec3& dir : {
        glm::ivec3(-1, 0, 0), glm::ivec3(1, 0, 0),
        glm::ivec3(0, 0, -1), glm::ivec3(0, 0, 1),
        glm::ivec3(0, -1, 0), glm::ivec3(0, 1, 0)
    })
    {
        ChunkPosition neighborPos = {
            pos.x + dir.x,
            pos.y,
            pos.z + dir.z
        };

        if (chunks.find(neighborPos) == chunks.end()) {
            return false;
        }
    }

    return true;
}

// Marks a specific neighboring chunk as dirty, indicating that it needs to be updated
void FlatWorld::markNeighborDirty(const ChunkPosition& pos, glm::ivec3 offset) {
    std::lock_guard<std::mutex> lock(chunkMutex);
    ChunkPosition neighborPos = {
        pos.x + offset.x,
        pos.y + offset.y,
        pos.z + offset.z
    };

    auto it = chunks.find(neighborPos);
    if (it != chunks.end()) {
        it->second->mesh.needsUpdate = true;
    }
}

// Updates the chunks around the player based on their position
void FlatWorld::updateChunksAroundPlayer(const glm::ivec3& playerChunk, const int VIEW_DISTANCE) {
    auto spiral = generateSpiralOffsets(VIEW_DISTANCE);
    int maxChunkPerFrame = 15;

    for (const auto& offset : spiral) {
        ChunkPosition pos = {
            playerChunk.x + offset.x,
            0,
            playerChunk.z + offset.y
        };

        if (chunks.find(pos) == chunks.end()) {
            chunkCreationQueue.push(pos);
        } else if (!chunks[pos]->mesh.shouldRender) {
            if (std::max(std::abs(offset.x), std::abs(offset.y)) <= VIEW_DISTANCE) {
                if (chunks[pos]->mesh.shouldRender == false) {
                    chunks[pos]->mesh.shouldRender = true;
                    chunkCreationQueue.push(pos);
                }
            }
        }
    }
}

// Generates a spiral pattern of offsets for chunk generation
std::vector<glm::ivec2> FlatWorld::generateSpiralOffsets(int radius) {
    std::vector<glm::ivec2> result;

    int x = 0, z = 0;
    int dx = 0, dz = -1;

    int sideLength = radius * 2 + 1;
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
void FlatWorld::uploadChunkMeshes(int maxPerFrame) {
    int uploadedChunks = 0;

    while (uploadedChunks < maxPerFrame) {
        std::shared_ptr<Chunk> chunkPtr = nullptr;
        if (!meshUploadQueue.tryPop(chunkPtr)) break;

        if (chunkPtr == nullptr || chunkPtr->mesh.isUploaded) break;
        try {
            uploadMeshToGPU(*chunkPtr);
        } catch (const std::exception& e) {
            std::cerr << "Caught exception during mesh upload: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Caught unknown exception during mesh upload." << std::endl;
        }
        
        uploadedChunks++;
        chunkPtr->mesh.needsUpdate = false;
        chunkPtr->mesh.isUploaded = true;
        chunkPtr->mesh.markedForUpload = false;
    }
}

// Uploads the mesh data to the GPU
void FlatWorld::uploadMeshToGPU(Chunk& chunk) {
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

// Unloads distant chunks based on the player's position and view distance
void FlatWorld::queueChunksForRemoval(const glm::ivec3& centerChunk, const int VIEW_DISTANCE) {
    std::unordered_set<ChunkPosition> shouldExist;

    for (int x = -VIEW_DISTANCE; x <= VIEW_DISTANCE; ++x) {
        for (int z = -VIEW_DISTANCE; z <= VIEW_DISTANCE; ++z) {
            shouldExist.insert({centerChunk.x + x, 0, centerChunk.z + z});
        }
    }

    std::vector<ChunkPosition> toRemove;
    for (const auto& [pos, chunk] : chunks) {
        if (shouldExist.count(pos) == 0) {
            toRemove.push_back(pos);
        }
    }

    for (const auto& pos : toRemove) {
        auto it = chunks.find(pos);
        if (it != chunks.end()) {
            if (it->second->mesh.markedForUpload) continue;
        
            std::shared_ptr<Chunk> chunkPtr = it->second;
            chunkPtr->mesh.shouldRender = false;
            chunkRemovalQueue.push(chunkPtr);
            chunks.erase(pos);
        }
        
    }
}

// Unloads distant chunks that are no longer needed
void FlatWorld::unloadDistantChunks() {

    std::shared_ptr<Chunk> chunkPtr = nullptr;
    if (!chunkRemovalQueue.tryPop(chunkPtr)) return;

    if (chunkPtr->mesh.isUploaded) {
        chunkPtr->mesh.VAO.deleteBuffers();
        chunkPtr->mesh.vaoInitialized = false;
        chunkPtr->mesh.vertices.clear();
        chunkPtr->mesh.indices.clear();
    }
}
