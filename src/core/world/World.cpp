#include "core/world/World.h"
#include "core/player/Player.h"

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

    std::cout << "\nJoining world generation threads..." << std::endl;
    for (auto& thread : chunkGenThreads) if (thread.joinable()) thread.join();
    for (auto& thread : meshGenThreads) if (thread.joinable()) thread.join();
    if (chunkManagerThread.joinable()) chunkManagerThread.join();

    std::cout << "Saving chunks to disk..." << std::endl;
    for (auto& [pos, chunk] : chunks) {
        if (!chunk) continue;

        try {
            saveChunkToFile(chunk);
        } catch (...) {
            std::cerr << "Exception while saving chunk at " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
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

    std::cout << "Saving player data..." << std::endl;
    try {
        savePlayerData(Player::instance(), Player::instance().getPlayerName());
    } catch (...) {
        std::cerr << "Failed to save player data." << std::endl;
    }
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

        if (loadChunkFromFile(pos, chunk)) {
            meshUploadQueue.push(chunk);
        } else {
            chunk = std::make_shared<Chunk>();
            chunk->setPosition(pos);
            chunk->generateTerrain();
            meshGenerationQueue.push(chunk);
        }
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
    int maxUnloads = Player::instance().VIEW_DISTANCE * 2;
    for (int i = 0; i < maxUnloads; ++i) {
        ChunkPosition pos;
        if (!chunkRemovalQueue.tryPop(pos)) break;

        if (std::abs(pos.x - Player::instance().getChunkPosition().x) <= Player::instance().VIEW_DISTANCE &&
            std::abs(pos.z - Player::instance().getChunkPosition().z) <= Player::instance().VIEW_DISTANCE) {
            continue;
        }

        auto it = chunks.find(pos);
        if (it == chunks.end()) continue;

        std::shared_ptr<Chunk> chunkPtr = it->second;
        if (!chunkPtr) continue;

        chunkSaveQueue.push(chunkPtr->makeSavableCopy());

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
                if (blockID != 0 && !BlockRegister::instance().blocks[blockID].isTransparent) {
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
    glm::vec3 playerCenter = player.getPosition();
    glm::vec3 playerHalfSize = player.playerSize * 0.5f;

    return !(
        playerCenter.x + playerHalfSize.x < blockCenter.x - blockHalfSize.x ||
        playerCenter.x - playerHalfSize.x > blockCenter.x + blockHalfSize.x ||
        playerCenter.y + playerHalfSize.y < blockCenter.y - blockHalfSize.y ||
        playerCenter.y - playerHalfSize.y > blockCenter.y + blockHalfSize.y ||
        playerCenter.z + playerHalfSize.z < blockCenter.z - blockHalfSize.z ||
        playerCenter.z - playerHalfSize.z > blockCenter.z + blockHalfSize.z
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
    saveDirectory = Game::instance().getSavePath() + saveDir;
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

    if (!std::filesystem::exists(filePath)) return false;

    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open player data file for reading: " << filePath << std::endl;
        return false;
    }

    using json = nlohmann::json;
    json playerData;
    file >> playerData;

    player.setPosition({
        playerData["position"]["x"].get<float>(),
        playerData["position"]["y"].get<float>(),
        playerData["position"]["z"].get<float>()
    });

    player.getCamera().yaw = playerData["rotation"]["yaw"].get<float>();
    player.getCamera().pitch = playerData["rotation"]["pitch"].get<float>();

    player.gameMode = playerData["gameMode"].get<int>();

    return true;
}

// Returns a players identifier
std::string World::getPlayerID() const {
    return Player::instance().getPlayerName();
}

// Unused WIP
// Reloads all chunks on keypress
void World::chunkReset() {
    if (!needsFullReset.exchange(false)) return;
}