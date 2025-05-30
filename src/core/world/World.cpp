#include "core/world/World.h"
#include "core/player/Player.h"
#include "network/Network.h"
#include "network/UDPSocket.h"
#include "network/Serializer.h"

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

World::World(const std:: string& saveDir) : saveDirectory(saveDir) {}

World::~World() {}

// Shuts down the world and stops all threads
void World::shutdown() {
    running = false;

    chunkCreationQueue.stop();
    meshGenerationQueue.stop();
    meshUploadQueue.stop();
    chunkRemovalQueue.stop();
    chunkUploadQueue.stop();
    meshUpdateQueue.stop();
    chunkSaveQueue.stop();

    std::cout << "\nJoining chunk generation threads..." << std::endl;
    for (auto& thread : chunkGenThreads) if (thread.joinable()) thread.join();
    std::cout << "Joining chunk meshing threads..." << std::endl;
    for (auto& thread : meshGenThreads) if (thread.joinable()) thread.join();
    std::cout << "Joining chunk manager thread..." << std::endl;
    if (chunkManagerThread.joinable()) chunkManagerThread.join();
    std::cout << "Joining network thread..." << std::endl;
    if (networkThread.joinable()) networkThread.join();

    if (!NetworkManager::instance().isOnlineMode() || NetworkManager::instance().isHost()) std::cout << "Saving chunks to disk..." << std::endl;
    for (auto& [pos, chunk] : chunks) {
        if (!chunk) continue;

        if (!NetworkManager::instance().isOnlineMode() || NetworkManager::instance().isHost()) {
            try {
                saveChunkToFile(chunk);
            } catch (...) {
                std::cerr << "Exception while saving chunk at " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
            }
        }

        if (chunk->mesh.isUploaded) {
            try {
                chunk->mesh.VAO.deleteBuffers();
            } catch (...) {
                std::cerr << "Exception in deleteBuffers for chunk at " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
            }
            chunk->mesh.isUploaded = false;
        }
        chunk->mesh.vertices.clear();
        chunk->mesh.indices.clear();
    }

    chunks.clear();

    if (!NetworkManager::instance().isOnlineMode() || NetworkManager::instance().isHost()) {
        std::cout << "Saving player data..." << std::endl;
        try {
            savePlayerData(Player::instance(), Player::instance().getPlayerName());
        } catch (...) {
            std::cerr << "Failed to save player data." << std::endl;
        }
    }
}

// Initializes the world by starting the chunk generation and meshing threads
void World::init() {
    BiomeNoise::initializeNoiseGenerator();

    if (running) return;
    running = true;

    int threads = std::thread::hardware_concurrency();
    
    for (int i = 0; i < (threads / 2) - 1; ++i) {
        chunkGenThreads.emplace_back(&World::chunkWorkerThread, this);
    }

    for (int i = 0; i < (threads / 2) - 1; ++i) {
        meshGenThreads.emplace_back(&World::meshWorkerThread, this);
    }

    chunkManagerThread = std::thread(&World::managerThread, this);
    if ((NetworkManager::instance().isClient() || NetworkManager::instance().isHost()) && NetworkManager::instance().isOnlineMode()) {
        tcpSocket = NetworkManager::instance().getTCPSocket();
        networkThread = std::thread(&World::chunkUpdateThread, this);
    } 
}

// Thread function for handeling chunk updates over the network
void World::chunkUpdateThread() {
    while (running) {
        if (udpReceiving.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        std::vector<uint8_t> buffer;
        Address from;

        if (UDPSocket::instance().receiveFrom(buffer, from)) {
            try {
                Message msg = Message::deserialize(buffer);
                if (msg.type == MessageType::ClientChunkUpdate) {
                    size_t offset = 0;
                    int32_t x = Serializer::readInt32(msg.data, offset);
                    int32_t y = Serializer::readInt32(msg.data, offset);
                    int32_t z = Serializer::readInt32(msg.data, offset);
                    ChunkPosition pos{x, y, z};

                    int32_t compressedSize = Serializer::readInt32(msg.data, offset);
                    if (msg.data.size() - offset < static_cast<size_t>(compressedSize)) {
                        std::cerr << "[Client] Invalid chunk update payload\n";
                        continue;
                    }

                    std::vector<uint8_t> compressed(msg.data.begin() + offset,
                                                    msg.data.begin() + offset + compressedSize);

                    size_t decompressedSize = ZSTD_getFrameContentSize(compressed.data(), compressed.size());
                    if (decompressedSize == ZSTD_CONTENTSIZE_ERROR || decompressedSize == ZSTD_CONTENTSIZE_UNKNOWN) {
                        std::cerr << "[Client] Could not determine chunk size\n";
                        continue;
                    }

                    std::vector<uint8_t> decompressed(decompressedSize);
                    size_t result = ZSTD_decompress(decompressed.data(), decompressedSize,
                                                    compressed.data(), compressed.size());
                    if (ZSTD_isError(result)) {
                        std::cerr << "[Client] Decompression failed: " << ZSTD_getErrorName(result) << "\n";
                        continue;
                    }

                    std::shared_ptr<Chunk> chunk = deserializeChunk(decompressed);
                    meshUploadQueue.push(chunk);
        
                }
            } catch (...) {
                std::cerr << "[Client] Failed to parse UDP message\n";
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
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

            updateChunksAroundPlayer(current, Player::instance().getViewDistance());
        }
        queueChunksForRemoval(current, Player::instance().getViewDistance() + 1);

        SavableChunk savableChunk;
        while (chunkSaveQueue.tryPop(savableChunk)) {
            std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>();
            chunk->setPosition(savableChunk.position);
            chunk->setBlocks(savableChunk.blocks);
            chunk->mesh.vertices = savableChunk.vertices;
            chunk->mesh.indices = savableChunk.indices;

            chunk->mesh.isEmpty = chunk->mesh.vertices.empty() && chunk->mesh.indices.empty();

            saveChunkToFile(chunk);
        }

        // pollTCPMessages();
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

        if (NetworkManager::instance().isClient() && NetworkManager::instance().isOnlineMode()) {
            if (requestChunkOverUDP(pos, chunk)) {
                meshUploadQueue.push(chunk);
            } else {
                chunk->generateTerrain();
                meshGenerationQueue.push(chunk);
            }
            continue;

        } else if (loadChunkFromFile(pos, chunk)) {
            meshUploadQueue.push(chunk);

        } else {
            chunk->setPosition(pos);
            chunk->generateTerrain();
            meshGenerationQueue.push(chunk);
        }
    }
}

// Thread function for meshing chunks that have been generated
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
        if (std::abs(chunk->getPosition().x - Player::instance().getChunkPosition().x) > Player::instance().getViewDistance() ||
            std::abs(chunk->getPosition().z - Player::instance().getChunkPosition().z) > Player::instance().getViewDistance()) {
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
    
            return 0;
        });
    
    if (!chunk->mesh.isEmpty && chunk->mesh.needsUpdate) {
        chunk->mesh.stagingVertices = std::move(vertices);
        chunk->mesh.stagingIndices = std::move(indices);
        chunk->mesh.hasNewMesh = true;
        chunk->mesh.isEmpty = chunk->mesh.stagingVertices.empty() && chunk->mesh.stagingIndices.empty();

    } else {
        chunk->mesh.vertices = std::move(vertices);
        chunk->mesh.indices = std::move(indices);
        chunk->mesh.isEmpty = chunk->mesh.vertices.empty() && chunk->mesh.indices.empty();
    }
    
    chunk->mesh.needsUpdate = false;

    if (!chunk->mesh.isEmpty) {
        meshUploadQueue.push(chunk);
    } else if (chunk->mesh.isUploaded && chunk->mesh.hasNewMesh) {
        chunk->mesh.VAO.deleteBuffers();
        chunk->mesh.isUploaded = false;
        chunk->mesh.vertices = std::move(chunk->mesh.stagingVertices);
        chunk->mesh.indices  = std::move(chunk->mesh.stagingIndices);
        chunk->mesh.stagingIndices.clear();
        chunk->mesh.stagingVertices.clear();
    }

    if ((!chunk->mesh.isEmpty || chunk->mesh.hasNewMesh) && NetworkManager::instance().isOnlineMode()) sendChunkOverUDP(chunk->makeSavableCopy());
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
    chunk->mesh.isUploaded = true;   

    chunk->mesh.needsUpdate = true;
    meshUpdateQueue.push(chunk);
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
        if (!meshUploadQueue.tryPop(chunk) || !chunk || (chunk->mesh.isUploaded && !chunk->mesh.hasNewMesh)) continue;
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
    if (chunk.mesh.isEmpty) return;

    if (chunk.mesh.isUploaded && chunk.mesh.hasNewMesh) {
        chunk.mesh.VAO.deleteBuffers();
        chunk.mesh.isUploaded = false;
        chunk.mesh.vertices = std::move(chunk.mesh.stagingVertices);
        chunk.mesh.indices  = std::move(chunk.mesh.stagingIndices);
        chunk.mesh.stagingIndices.clear();
        chunk.mesh.stagingVertices.clear();
    }
    
    if (!chunk.mesh.vertices.empty()) {
        chunk.mesh.VAO.init();
        chunk.mesh.VAO.bind();
        chunk.mesh.VAO.addVertexBuffer(chunk.mesh.vertices);
        chunk.mesh.VAO.addElementBuffer(chunk.mesh.indices);
        chunk.mesh.VAO.addAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        chunk.mesh.VAO.addAttribute(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        chunk.mesh.VAO.addAttribute(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

        chunk.mesh.hasNewMesh = false;
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
    int maxUnloads = Player::instance().getViewDistance() * 2;
    for (int i = 0; i < maxUnloads; ++i) {
        ChunkPosition pos;
        if (!chunkRemovalQueue.tryPop(pos)) break;

        if (std::abs(pos.x - Player::instance().getChunkPosition().x) <= Player::instance().getViewDistance() &&
            std::abs(pos.z - Player::instance().getChunkPosition().z) <= Player::instance().getViewDistance()) {
            continue;
        }

        auto it = chunks.find(pos);
        if (it == chunks.end()) continue;

        std::shared_ptr<Chunk> chunkPtr = it->second;
        if (!chunkPtr) continue;

        if (!NetworkManager::instance().isOnlineMode || NetworkManager::instance().isHost()) {
            chunkSaveQueue.push(chunkPtr->makeSavableCopy());
        }

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

bool World::collidesWithBlockAABB(glm::vec3 pos, glm::vec3 size) const {
    glm::ivec3 min = glm::floor(pos - size * 0.5f);
    glm::ivec3 max = glm::floor(pos + size * 0.5f);

    for (int x = min.x; x <= max.x; ++x) {
        for (int y = min.y; y <= max.y; ++y) {
            for (int z = min.z; z <= max.z; ++z) {
                int blockID = getBlockIDAtWorldPosition(x, y, z);
                if (blockID != 0 && BlockRegister::instance().blocks[blockID].isSolid) {
                    return true;
                }
            }
        }
    }
    return false;
}

#include "core/player/Player.h"

bool World::wouldBlockOverlapPlayer(const glm::ivec3& blockPos) const {
    glm::vec3 blockCenter = glm::vec3(blockPos) + 0.5f;
    glm::vec3 blockHalfSize = glm::vec3(0.5f);
    
    const Player& player = Player::instance();
    glm::vec3 playerCenter = player.getPosition() - glm::vec3(0.0f, player.eyeOffset, 0.0f);
    glm::vec3 playerHalfSize = player.playerSize * 0.5f;

    return !(
        playerCenter.x + playerHalfSize.x <= blockCenter.x - blockHalfSize.x ||
        playerCenter.x - playerHalfSize.x >= blockCenter.x + blockHalfSize.x ||
        playerCenter.y + playerHalfSize.y <= blockCenter.y - blockHalfSize.y ||
        playerCenter.y - playerHalfSize.y >= blockCenter.y + blockHalfSize.y ||
        playerCenter.z + playerHalfSize.z <= blockCenter.z - blockHalfSize.z ||
        playerCenter.z - playerHalfSize.z >= blockCenter.z + blockHalfSize.z
    );
}

// Saves the chunk to a file
void World::saveChunkToFile(const std::shared_ptr<Chunk>& chunk) {
    const ChunkPosition& pos = chunk->getPosition();

    if (chunk->mesh.isEmpty) return;

    std::ostringstream oss;
    oss << saveDirectory << "/chunks/" << pos.x << "_" << pos.y << "_" << pos.z << ".zst";
    std::string filename = oss.str();

    std::vector<char> buffer;

    // Block Data
    const auto& blocks = chunk->getBlocks();
    uint32_t blockCount = blocks.size();
    buffer.insert(buffer.end(), reinterpret_cast<const char*>(&blockCount), reinterpret_cast<const char*>(&blockCount) + sizeof(uint32_t));
    buffer.insert(buffer.end(), reinterpret_cast<const char*>(blocks.data()), reinterpret_cast<const char*>(blocks.data()) + blockCount * sizeof(uint16_t));

    // Chunk mesh Verts
    const auto& verts = chunk->mesh.vertices;
    uint32_t vertCount = verts.size();
    buffer.insert(buffer.end(), reinterpret_cast<const char*>(&vertCount), reinterpret_cast<const char*>(&vertCount) + sizeof(uint32_t));
    buffer.insert(buffer.end(), reinterpret_cast<const char*>(verts.data()), reinterpret_cast<const char*>(verts.data()) + vertCount * sizeof(Vertex));

    // Chunk mesh indices
    const auto& indices = chunk->mesh.indices;
    uint32_t indexCount = indices.size();
    buffer.insert(buffer.end(), reinterpret_cast<const char*>(&indexCount), reinterpret_cast<const char*>(&indexCount) + sizeof(uint32_t));
    buffer.insert(buffer.end(), reinterpret_cast<const char*>(indices.data()), reinterpret_cast<const char*>(indices.data()) + indexCount * sizeof(GLuint));

    size_t maxSize = ZSTD_compressBound(buffer.size());
    std::vector<char> compressedBuffer(maxSize);
    size_t compressedSize = ZSTD_compress(compressedBuffer.data(), maxSize, buffer.data(), buffer.size(), 1);

    if (ZSTD_isError(compressedSize)) {
        std::cerr << "ZSTD compression failed: " << ZSTD_getErrorName(compressedSize) << std::endl;
        return;
    }

    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    out.write(compressedBuffer.data(), compressedSize);
    out.close();
}

// Loads the chunk from a file
bool World::loadChunkFromFile(const ChunkPosition& pos, std::shared_ptr<Chunk>& chunkOut) {
    std::ostringstream oss;
    oss << saveDirectory << "/chunks/" << pos.x << "_" << pos.y << "_" << pos.z << ".zst";
    std::string filename = oss.str();

    if (!std::filesystem::exists(filename)) return false;

    std::ifstream in(filename, std::ios::binary | std::ios::ate);
    if (!in.is_open()) {
        std::cerr << "Failed to open file for reading: " << filename << std::endl;
        return false;
    }

    std::streamsize fileSize = in.tellg();
    in.seekg(0);
    std::vector<char> compressed(fileSize);
    in.read(compressed.data(), fileSize);
    in.close();

    size_t decompressedSize = ZSTD_getFrameContentSize(compressed.data(), fileSize);
    if (decompressedSize == ZSTD_CONTENTSIZE_ERROR || decompressedSize == ZSTD_CONTENTSIZE_UNKNOWN) {
        std::cerr << "ZSTD: unknown decompressed size\n";
        return false;
    }

    std::vector<char> buffer(decompressedSize);
    size_t result = ZSTD_decompress(buffer.data(), decompressedSize, compressed.data(), fileSize);
    if (ZSTD_isError(result)) {
        std::cerr << "ZSTD decompression failed: " << ZSTD_getErrorName(result) << std::endl;
        return false;
    }

    size_t offset = 0;
    auto read = [&](void* dst, size_t size) {
        std::memcpy(dst, buffer.data() + offset, size);
        offset += size;
    };

    chunkOut = std::make_shared<Chunk>();
    chunkOut->setPosition(pos);

    uint32_t blockCount;
    read(&blockCount, sizeof(uint32_t));
    std::array<uint16_t, CHUNK_VOLUME> blocks;
    read(blocks.data(), blockCount * sizeof(uint16_t));
    chunkOut->setBlocks(blocks);

    uint32_t vertCount;
    read(&vertCount, sizeof(uint32_t));
    chunkOut->mesh.vertices.resize(vertCount);
    read(chunkOut->mesh.vertices.data(), vertCount * sizeof(Vertex));

    uint32_t indexCount;
    read(&indexCount, sizeof(uint32_t));
    chunkOut->mesh.indices.resize(indexCount);
    read(chunkOut->mesh.indices.data(), indexCount * sizeof(GLuint));

    chunkOut->mesh.isEmpty = chunkOut->mesh.vertices.empty() && chunkOut->mesh.indices.empty();
    chunkOut->mesh.needsUpdate = false;
    chunkOut->mesh.isUploaded = false;
    
    return true;
}

#include "core/game/Game.h"

// Sets the save directory for the world
void World::setSaveDirectory(const std::string& saveDir) {
    saveDirectory = (std::filesystem::path(Game::instance().getSavePath()) / saveDir).string();
}

// Creates the save directory if it doesn't exist
void World::createSaveDirectory() {
    if (std::filesystem::exists(saveDirectory)) return;
    std::filesystem::create_directories(saveDirectory);
    std::filesystem::create_directories(saveDirectory + "/chunks");
    std::filesystem::create_directories(saveDirectory + "/players");
    createWorldConfigFile();
}

// Creates the world config file
void World::createWorldConfigFile() {
    if (std::filesystem::exists(saveDirectory + "/WorldConfig.json")) return;

    using json = nlohmann::json;

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    json config;
    config["created_at"] = std::string(std::ctime(&now_c));
    config["world_seed"] = seed;
    config["initial_version"] = Game::instance().getGameVersion();

    std::ofstream file(saveDirectory + "/WorldConfig.json");
    if (file.is_open()) {
        file << config.dump(4);
        file.close();
    }
}

// Saves the player data to a file
void World::savePlayerData(Player& player, const std::string& playerID) {
    std::string filePath = saveDirectory + "/players/" + getPlayerID() + ".json";
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open player data file for writing: " << filePath << std::endl;
        return;
    }

    using json = nlohmann::json;
    json playerData;

    playerData["position"] = {
        {"x", player.getPosition().x},
        {"y", player.getPosition().y},
        {"z", player.getPosition().z}
    }; 

    playerData["rotation"] = {
        {"yaw", player.getCamera().yaw},
        {"pitch", player.getCamera().pitch}
    };

    playerData["gameMode"] = player.gameMode;

    file << playerData.dump(4);
    file.close();
}

// Loads the player data from a file
bool World::loadPlayerData(Player& player, const std::string& playerID) {
    std::string filePath = saveDirectory + "/players/" + getPlayerID() + ".json";

    if (!std::filesystem::exists(filePath)) {
        int wx = 0;
        int wz = 0;
        int groundY = -1;

        for (int y = 256; y >= 0; --y) {
            ChunkPosition chunkPos = {
                (wx < 0 && wx % CHUNK_SIZE != 0) ? (wx / CHUNK_SIZE - 1) : (wx / CHUNK_SIZE),
                (y  < 0 && y  % CHUNK_SIZE != 0) ? (y  / CHUNK_SIZE - 1) : (y  / CHUNK_SIZE),
                (wz < 0 && wz % CHUNK_SIZE != 0) ? (wz / CHUNK_SIZE - 1) : (wz / CHUNK_SIZE)
            };

            if (chunks.find(chunkPos) == chunks.end()) {
                std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>();
                if (!loadChunkFromFile(chunkPos, chunk)) {
                    chunk->setPosition(chunkPos);
                    chunk->generateTerrain();
                    generateMesh(chunk);
                    uploadMeshToGPU(*chunk);
                }
                chunks[chunkPos] = chunk;
            }

            int blockID = getBlockIDAtWorldPosition(wx, y, wz);
            if (blockID != 0 && BlockRegister::instance().blocks[blockID].isSolid) {
                groundY = y + 1;
                break;
            }
        }

        if (groundY == -1) groundY = 64;

        player.setPosition({
            static_cast<float>(wx),
            static_cast<float>(groundY) + player.playerSize.y * 0.5f + 0.01f,
            static_cast<float>(wz)
        });

        player.getCamera().yaw = 0.0f;
        player.getCamera().pitch = 0.0f;
        player.gameMode = 0;

        return true;
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open player data file for reading: " << filePath << std::endl;
        return false;
    }

    using json = nlohmann::json;
    json playerData;
    file >> playerData;

    glm::vec3 pos {
        playerData["position"]["x"].get<float>(),
        playerData["position"]["y"].get<float>(),
        playerData["position"]["z"].get<float>()
    };

    int wx = static_cast<int>(std::floor(pos.x));
    int wz = static_cast<int>(std::floor(pos.z));
    int startY = static_cast<int>(std::floor(pos.y));
    int groundY = -1;

    for (int y = startY; y >= 0; --y) {
        ChunkPosition chunkPos = {
            (wx < 0 && wx % CHUNK_SIZE != 0) ? (wx / CHUNK_SIZE - 1) : (wx / CHUNK_SIZE),
            (y   < 0 && y   % CHUNK_SIZE != 0) ? (y   / CHUNK_SIZE - 1) : (y   / CHUNK_SIZE),
            (wz < 0 && wz % CHUNK_SIZE != 0) ? (wz / CHUNK_SIZE - 1) : (wz / CHUNK_SIZE)
        };

        if (chunks.find(chunkPos) == chunks.end()) {
            std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>();
            if (!loadChunkFromFile(chunkPos, chunk)) {
                chunk->setPosition(chunkPos);
                chunk->generateTerrain();
                generateMesh(chunk);
                uploadMeshToGPU(*chunk);
            }
            chunks[chunkPos] = chunk;
        }

        int blockID = getBlockIDAtWorldPosition(wx, y, wz);
        if (blockID != 0 && BlockRegister::instance().blocks[blockID].isSolid) {
            groundY = y + 1;
            break;
        }
    }

    if (groundY == -1) groundY = startY;

    player.setPosition({
        pos.x,
        static_cast<float>(groundY) + player.playerSize.y * 0.5f + 0.01f,
        pos.z
    });

    player.getCamera().setRotation(
        playerData["rotation"]["yaw"].get<float>(),
        playerData["rotation"]["pitch"].get<float>()
    );
    player.gameMode = playerData["gameMode"].get<int>();

    return true;
}

// Returns a players identifier
std::string World::getPlayerID() const {
    return Player::instance().getPlayerName();
}

#include "network/Serializer.h"

void World::serializeChunk(SavableChunk& chunk, std::vector<uint8_t>& out) {
    out.clear();

    // Position
    Serializer::writeInt32(out, chunk.position.x);
    Serializer::writeInt32(out, chunk.position.y);
    Serializer::writeInt32(out, chunk.position.z);

    // Block Data
    const auto& blocks = chunk.blocks;
    Serializer::writeInt32(out, static_cast<int32_t>(blocks.size()));
    out.insert(out.end(),
        reinterpret_cast<const uint8_t*>(blocks.data()),
        reinterpret_cast<const uint8_t*>(blocks.data()) + blocks.size() * sizeof(uint16_t));

    // Vertices
    const auto& verts = chunk.vertices;
    Serializer::writeInt32(out, static_cast<int32_t>(verts.size()));
    out.insert(out.end(),
        reinterpret_cast<const uint8_t*>(verts.data()),
        reinterpret_cast<const uint8_t*>(verts.data()) + verts.size() * sizeof(Vertex));

    // Indices
    const auto& indices = chunk.indices;
    Serializer::writeInt32(out, static_cast<int32_t>(indices.size()));
    out.insert(out.end(),
        reinterpret_cast<const uint8_t*>(indices.data()),
        reinterpret_cast<const uint8_t*>(indices.data()) + indices.size() * sizeof(GLuint));
}

std::shared_ptr<Chunk> World::deserializeChunk(const std::vector<uint8_t>& in) {
    size_t offset = 0;

    int x = Serializer::readInt32(in, offset);
    int y = Serializer::readInt32(in, offset);
    int z = Serializer::readInt32(in, offset);
    ChunkPosition pos{x, y, z};

    auto chunk = std::make_shared<Chunk>();
    chunk->setPosition(pos);

    // Block data
    int blockCount = Serializer::readInt32(in, offset);
    std::array<uint16_t, CHUNK_VOLUME> blocks{};
    std::memcpy(blocks.data(), in.data() + offset, blockCount * sizeof(uint16_t));
    offset += blockCount * sizeof(uint16_t);
    chunk->setBlocks(blocks);

    // Vertices
    int vertCount = Serializer::readInt32(in, offset);
    chunk->mesh.vertices.resize(vertCount);
    std::memcpy(chunk->mesh.vertices.data(), in.data() + offset, vertCount * sizeof(Vertex));
    offset += vertCount * sizeof(Vertex);

    // Indices
    int indexCount = Serializer::readInt32(in, offset);
    chunk->mesh.indices.resize(indexCount);
    std::memcpy(chunk->mesh.indices.data(), in.data() + offset, indexCount * sizeof(GLuint));
    offset += indexCount * sizeof(GLuint);

    chunk->mesh.isEmpty = chunk->mesh.vertices.empty() && chunk->mesh.indices.empty();
    chunk->mesh.needsUpdate = false;
    chunk->mesh.isUploaded = false;

    return chunk;
}

void World::networkWorker(ChunkPosition pos) {
    if (tcpSocket == INVALID_SOCKET) {
        std::cerr << "[Client] Invalid TCP socket\n";
        return;
    }

    Message request;
    request.type = MessageType::ChunkRequest;

    Serializer::writeInt32(request.data, pos.x);
    Serializer::writeInt32(request.data, pos.y);
    Serializer::writeInt32(request.data, pos.z);

    std::vector<uint8_t> serialized = request.serialize();
    if (serialized.empty()) {
        std::cerr << "[Client] ERROR: Serialized message is empty!\n";
    }
    std::cout << "[Client] Sending message of length " << serialized.size() << "\n";
    uint32_t length = static_cast<uint32_t>(serialized.size());
    std::vector<uint8_t> prefix(4);
    std::memcpy(prefix.data(), &length, 4);

    TCPSocket::sendAll(tcpSocket, prefix);
    TCPSocket::sendAll(tcpSocket, serialized);

    std::vector<uint8_t> lenBuf;
    if (!TCPSocket::recvAll(tcpSocket, lenBuf, 4)) {
        std::cerr << "[Client] Failed to read chunk response size\n";
        return;
    }

    uint32_t responseLength;
    std::memcpy(&responseLength, lenBuf.data(), 4);
    std::vector<uint8_t> payload;
    if (!TCPSocket::recvAll(tcpSocket, payload, responseLength)) {
        std::cerr << "[Client] Failed to read full chunk response\n";
        return;
    }

    try {
        Message response = Message::deserialize(payload);
        if (response.type == MessageType::ChunkData) {
            auto chunk = World::instance().deserializeChunk(response.data);
            meshUploadQueue.push(chunk);
        } else if (response.type == MessageType::ChunkNotFound) {
            auto chunk = std::make_shared<Chunk>();
            chunk->setPosition(pos);
            chunk->generateTerrain();
            meshGenerationQueue.push(chunk);

            Message generated;
            generated.type = MessageType::ChunkGeneratedByClient;
            std::vector<uint8_t> sendBack = generated.serialize();
            uint32_t sendBackLen = static_cast<uint32_t>(sendBack.size());
            std::vector<uint8_t> sendBackPrefix(4);

            std::memcpy(sendBackPrefix.data(), &sendBackLen, 4);
            TCPSocket::sendAll(tcpSocket, sendBackPrefix);
            TCPSocket::sendAll(tcpSocket, sendBack);

            return;
        }
    } catch (...) {
        std::cerr << "[Client] Failed to parse server response message\n";
        return;
    }
}

bool World::requestChunkOverUDP(const ChunkPosition& pos, std::shared_ptr<Chunk>& outChunk) {
    udpReceiving.store(true, std::memory_order_relaxed);

    struct ResetGuard {
        World* world;
        ResetGuard(World* w) : world(w) {}
        ~ResetGuard() { world->udpReceiving.store(false, std::memory_order_relaxed); }
    } guard(this);

    Message request;
    request.type = MessageType::ChunkRequest;
    Serializer::writeInt32(request.data, pos.x);
    Serializer::writeInt32(request.data, pos.y);
    Serializer::writeInt32(request.data, pos.z);

    auto serialized = request.serialize();
    if (!UDPSocket::instance().sendTo(serialized, UDPSocket::instance().getAddress())) {
        std::cerr << "[Client] Failed to send UDP chunk request\n";
        return false;
    }

    std::vector<uint8_t> responseBuf;
    Address from;
    if (!UDPSocket::instance().receiveFrom(responseBuf, from)) {
        std::cerr << "[Client] UDP response timeout or error\n";
        return false;
    }

    try {
        Message response = Message::deserialize(responseBuf);
        if (response.type == MessageType::ChunkNotFound) {
            size_t offset = 0;
            int32_t x = Serializer::readInt32(response.data, offset);
            int32_t y = Serializer::readInt32(response.data, offset);
            int32_t z = Serializer::readInt32(response.data, offset);
            ChunkPosition missingPos{x, y, z};
            outChunk->setPosition(missingPos);
            return false;
        }

        if (response.type == MessageType::ClientChunkUpdate) {
            size_t offset = 0;
            int32_t x = Serializer::readInt32(response.data, offset);
            int32_t y = Serializer::readInt32(response.data, offset);
            int32_t z = Serializer::readInt32(response.data, offset);
            ChunkPosition pos{x, y, z};

            int32_t compressedSize = Serializer::readInt32(response.data, offset);

            if (response.data.size() - offset < static_cast<size_t>(compressedSize)) {
                std::cerr << "[Client] Invalid chunk update payload\n";
                return false;
            }
            
            std::vector<uint8_t> compressed(response.data.begin() + offset,
                                            response.data.begin() + offset + compressedSize);
            size_t decompressedSize = ZSTD_getFrameContentSize(compressed.data(), compressed.size());
            if (decompressedSize == ZSTD_CONTENTSIZE_ERROR || decompressedSize == ZSTD_CONTENTSIZE_UNKNOWN) {
                std::cerr << "[Client] Could not determine chunk size\n";
                return false;

            }
            
            std::vector<uint8_t> decompressed(decompressedSize);
            size_t result = ZSTD_decompress(decompressed.data(), decompressedSize,
                                            compressed.data(), compressed.size());
            if (ZSTD_isError(result)) {
                std::cerr << "[Client] Decompression failed: " << ZSTD_getErrorName(result) << "\n";
                return false;
            }
            
            std::shared_ptr<Chunk> chunk = deserializeChunk(decompressed);
            meshUploadQueue.push(chunk);
        }

        if (response.type != MessageType::ChunkData) return false;

        size_t offset = 0;
        int32_t x = Serializer::readInt32(response.data, offset);
        int32_t y = Serializer::readInt32(response.data, offset);
        int32_t z = Serializer::readInt32(response.data, offset);
        int32_t compressedSize = Serializer::readInt32(response.data, offset);

        if (response.data.size() - offset < static_cast<size_t>(compressedSize)) {
            std::cerr << "[Client] Compressed chunk data incomplete\n";
            return false;
        }

        std::vector<uint8_t> compressedData(response.data.begin() + offset,
                                            response.data.begin() + offset + compressedSize);

        size_t decompressedSize = ZSTD_getFrameContentSize(compressedData.data(), compressedData.size());
        if (decompressedSize == ZSTD_CONTENTSIZE_ERROR || decompressedSize == ZSTD_CONTENTSIZE_UNKNOWN) {
            std::cerr << "[Client] Failed to get decompressed size\n";
            return false;
        }

        std::vector<uint8_t> decompressed(decompressedSize);
        size_t result = ZSTD_decompress(decompressed.data(), decompressedSize,
                                        compressedData.data(), compressedData.size());
        if (ZSTD_isError(result)) {
            std::cerr << "[Client] Failed to decompress chunk: "
                      << ZSTD_getErrorName(result) << "\n";
            return false;
        }

        outChunk = deserializeChunk(decompressed);
        return true;

    } catch (...) {
        std::cerr << "[Client] Failed to parse UDP chunk response\n";
        return false;
    }
}

void World::sendChunkOverUDP(SavableChunk chunk) {
    Message message;
    if (chunk.hasMeshUpdate) {
        message.type = MessageType::ClientChunkUpdate;
    } else {
        message.type = MessageType::ChunkGeneratedByClient;
    }

    Serializer::writeInt32(message.data, chunk.position.x);
    Serializer::writeInt32(message.data, chunk.position.y);
    Serializer::writeInt32(message.data, chunk.position.z);

    std::vector<uint8_t> rawChunkData;
    serializeChunk(chunk, rawChunkData);

    size_t maxSize = ZSTD_compressBound(rawChunkData.size());
    std::vector<char> compressed(maxSize);
    size_t compressedSize = ZSTD_compress(compressed.data(), maxSize,
                                          rawChunkData.data(), rawChunkData.size(), 1);

    if (ZSTD_isError(compressedSize)) {
        std::cerr << "ZSTD compression failed: " << ZSTD_getErrorName(compressedSize) << std::endl;
        return;
    }

    Serializer::writeInt32(message.data, static_cast<int32_t>(compressedSize));
    message.data.insert(message.data.end(), compressed.begin(), compressed.begin() + compressedSize);

    std::vector<uint8_t> packet = message.serialize();
    if (packet.size() > 65507) {
        std::cerr << "[Client] Chunk too large to send over UDP\n";
        return;
    }

    if (!UDPSocket::instance().sendTo(packet, NetworkManager::instance().getAddress())) {
        std::cerr << "[Client] Failed to send chunk at " << chunk.position.x << ", "
                  << chunk.position.y << ", " << chunk.position.z << " over UDP\n";
    }
}

void World::sendChunkUpdate(SavableChunk chunk) {
    Message message;
    message.type = MessageType::ClientChunkUpdate;

    Serializer::writeInt32(message.data, chunk.position.x);
    Serializer::writeInt32(message.data, chunk.position.y);
    Serializer::writeInt32(message.data, chunk.position.z);

    std::vector<uint8_t> rawChunkData;
    serializeChunk(chunk, rawChunkData);

    size_t maxSize = ZSTD_compressBound(rawChunkData.size());
    std::vector<char> compressed(maxSize);
    size_t compressedSize = ZSTD_compress(compressed.data(), maxSize,
                                          rawChunkData.data(), rawChunkData.size(), 1);

    if (ZSTD_isError(compressedSize)) {
        std::cerr << "ZSTD compression failed: " << ZSTD_getErrorName(compressedSize) << std::endl;
        return;
    }

    Serializer::writeInt32(message.data, static_cast<int32_t>(compressedSize));
    message.data.insert(message.data.end(), compressed.begin(), compressed.begin() + compressedSize);

    std::vector<uint8_t> packet = message.serialize();

    if (chunk.hasMeshUpdate) {
        uint32_t len = static_cast<uint32_t>(packet.size());
        std::vector<uint8_t> lengthPrefix(4);
        std::memcpy(lengthPrefix.data(), &len, 4);

        if (!TCPSocket::sendAll(tcpSocket, lengthPrefix) || !TCPSocket::sendAll(tcpSocket, packet)) {
            std::cerr << "[Client] Failed to send chunk update over TCP\n";
        } else {
            std::cout << "[Client] Sent chunk update over TCP\n";
        }
    } else {
        if (packet.size() > 65507) {
            std::cerr << "[Client] Chunk too large to send over UDP\n";
            return;
        }

        if (!UDPSocket::instance().sendTo(packet, NetworkManager::instance().getAddress())) {
            std::cerr << "[Client] Failed to send chunk over UDP\n";
        }
    }
}

void World::pollTCPMessages() {
    if (tcpSocket == INVALID_SOCKET) return;

    u_long bytesAvailable = 0;
    ioctlsocket(tcpSocket, FIONREAD, &bytesAvailable);
    if (bytesAvailable < 4) return;

    std::vector<uint8_t> lenBuf(4);
    if (!TCPSocket::recvAll(tcpSocket, lenBuf, 4)) {
        std::cerr << "[Client] Failed to read TCP message length\n";
        return;
    }

    uint32_t msgLength;
    std::memcpy(&msgLength, lenBuf.data(), 4);
    if (msgLength == 0 || msgLength > 10 * 1024 * 1024) {
        std::cerr << "[Client] Invalid TCP message length: " << msgLength << "\n";
        return;
    }

    std::vector<uint8_t> msgData;
    if (!TCPSocket::recvAll(tcpSocket, msgData, msgLength)) {
        std::cerr << "[Client] Failed to read full TCP message\n";
        return;
    }

    try {
        Message msg = Message::deserialize(msgData);

        if (msg.type == MessageType::ClientChunkUpdate) {
            size_t offset = 0;
            int32_t x = Serializer::readInt32(msg.data, offset);
            int32_t y = Serializer::readInt32(msg.data, offset);
            int32_t z = Serializer::readInt32(msg.data, offset);
            ChunkPosition pos{x, y, z};

            int32_t compressedSize = Serializer::readInt32(msg.data, offset);
            if (msg.data.size() - offset < static_cast<size_t>(compressedSize)) {
                std::cerr << "[Client] Invalid chunk update payload\n";
                return;
            }

            std::vector<uint8_t> compressed(msg.data.begin() + offset, msg.data.begin() + offset + compressedSize);

            size_t decompressedSize = ZSTD_getFrameContentSize(compressed.data(), compressed.size());
            if (decompressedSize == ZSTD_CONTENTSIZE_ERROR || decompressedSize == ZSTD_CONTENTSIZE_UNKNOWN) {
                std::cerr << "[Client] Could not determine chunk size\n";
                return;
            }

            std::vector<uint8_t> decompressed(decompressedSize);
            size_t result = ZSTD_decompress(decompressed.data(), decompressedSize, compressed.data(), compressed.size());
            if (ZSTD_isError(result)) {
                std::cerr << "[Client] Decompression failed: " << ZSTD_getErrorName(result) << "\n";
                return;
            }

            std::shared_ptr<Chunk> chunk = deserializeChunk(decompressed);
            std::cout << "[Client] Received chunk update for " << pos.x << ", " << pos.y << ", " << pos.z << "\n";
            meshUploadQueue.push(chunk);
        }

    } catch (...) {
        std::cerr << "[Client] Failed to deserialize TCP message\n";
    }
}

// Unused WIP
// Reloads all chunks on keypress
void World::chunkReset() {
    if (!needsFullReset.exchange(false)) return;
}