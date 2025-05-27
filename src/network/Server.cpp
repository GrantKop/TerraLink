#include "network/Server.h"
#include "network/Message.h"
#include "network/UDPSocket.h"
#include "network/Serializer.h"

#include <iostream>
#include <zstd.h>
#include <zstd_errors.h>

#include <csignal>

std::atomic<bool> serverRunning = true;

void signalHandler(int signum) {
    std::cout << "\nCaught signal " << signum << ", shutting down...\n";
    serverRunning = false;
}

Server::Server(uint16_t listenPort) {
    if (!listener.bind(listenPort)) {
        std::cerr << "[Server] Failed to bind TCP socket on port " << listenPort << std::endl;
        std::exit(1);
    }

    if (!listener.listen()) {
        std::cerr << "[Server] Failed to listen on TCP socket\n";
        std::exit(1);
    }
}

void Server::run() {
    clientSocket = listener.acceptClient();
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "[Server] Failed to accept TCP client\n";
        return;
    }
    std::cout << "[Server] TCP client connected!\n";

    handleTCPClient(clientSocket);
}

void Server::handleTCPClient(SOCKET socket) {
    std::signal(SIGINT, signalHandler);
    while (serverRunning) {
        std::vector<uint8_t> lengthBuf;
        if (!TCPSocket::recvAll(socket, lengthBuf, 4)) break;

        uint32_t msgLength;
        std::memcpy(&msgLength, lengthBuf.data(), 4);

        std::vector<uint8_t> data;
        if (!TCPSocket::recvAll(socket, data, msgLength)) break;

        try {
            Message msg;
            try {
                msg = Message::deserialize(data);
            } catch (const std::exception& e) {
                std::cerr << "[Server] Failed to deserialize message: " << e.what() << "\n";
                continue;
            }
            if (msg.type == MessageType::ChunkRequest) {
                size_t offset = 0;
                int32_t x = Serializer::readInt32(msg.data, offset);
                int32_t y = Serializer::readInt32(msg.data, offset);
                int32_t z = Serializer::readInt32(msg.data, offset);
                ChunkPosition pos{x, y, z};

                std::vector<uint8_t> chunkData;
                if (loadChunkFromFile(pos, chunkData)) {
                    Message response;
                    response.type = MessageType::ChunkData;
                    response.data = chunkData;
                
                    std::vector<uint8_t> serialized = response.serialize();
                
                    uint32_t len = static_cast<uint32_t>(serialized.size());
                    std::vector<uint8_t> lengthPrefix(4);
                    std::memcpy(lengthPrefix.data(), &len, 4);
                
                    TCPSocket::sendAll(socket, lengthPrefix);
                    TCPSocket::sendAll(socket, serialized);
                } else {
                    Message notFound;
                    notFound.type = MessageType::ChunkNotFound;
                    Serializer::writeInt32(notFound.data, pos.x);
                    Serializer::writeInt32(notFound.data, pos.y);
                    Serializer::writeInt32(notFound.data, pos.z);

                    std::vector<uint8_t> serialized = notFound.serialize();
                    uint32_t length = static_cast<uint32_t>(serialized.size());
                    std::vector<uint8_t> prefix(4);
                    std::memcpy(prefix.data(), &length, 4);

                    TCPSocket::sendAll(socket, prefix);
                    TCPSocket::sendAll(socket, serialized);
                }
            } else if (msg.type == MessageType::ChunkGeneratedByClient) {
                std::shared_ptr<Chunk> chunk = deserializeChunk(msg.data);
                saveChunkToFile(chunk);
            }      
        } catch (...) {
            std::cerr << "[Server] Failed to parse incoming TCP chunk request\n";
        }
    }

    std::cerr << "[Server] Client disconnected\n";
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}

void Server::handleMessage(const Message& msg, const Address& from) {
    if (msg.type == MessageType::ChunkRequest) {
        size_t offset = 0;
        int32_t x = Serializer::readInt32(msg.data, offset);
        int32_t y = Serializer::readInt32(msg.data, offset);
        int32_t z = Serializer::readInt32(msg.data, offset);

        ChunkPosition pos{x, y, z};
        chunkRequestQueue.push({pos, from});
    }
    else if (msg.type == MessageType::ChunkGeneratedByClient) {
        std::shared_ptr<Chunk> chunk = deserializeChunk(msg.data);
        saveChunkToFile(chunk);
    }
    else if (msg.type == MessageType::Ping) {
        std::cout << "[Server] Received ping from " << from.ip << ":" << from.port << std::endl;
        Message pong;
        pong.type = MessageType::Pong;
        socket.sendTo(pong.serialize(), from);
    }
}

void Server::handlePendingRequests() {
    PendingRequest req;
    while (chunkRequestQueue.tryPop(req)) {
        std::vector<uint8_t> chunkData;
        if (loadChunkFromFile(req.pos, chunkData)) {
            Message response;
            response.type = MessageType::ChunkData;
            response.data = chunkData;
            socket.sendTo(response.serialize(), req.client);
        } else {
            Message notFound;
            notFound.type = MessageType::ChunkNotFound;
            Serializer::writeInt32(notFound.data, req.pos.x);
            Serializer::writeInt32(notFound.data, req.pos.y);
            Serializer::writeInt32(notFound.data, req.pos.z);
            socket.sendTo(notFound.serialize(), req.client);
        }
    }
}

bool Server::loadChunkFromFile(const ChunkPosition& pos, std::vector<uint8_t>& outData) {
    std::ostringstream oss;
    oss << std::filesystem::current_path().parent_path().parent_path().string() << "/saves/world/chunks/" << pos.x << "_" << pos.y << "_" << pos.z << ".zst";
    std::string filename = oss.str();

    if (!std::filesystem::exists(filename)) return false;

    std::ifstream in(filename, std::ios::binary | std::ios::ate);
    if (!in.is_open()) return false;

    std::streamsize fileSize = in.tellg();
    in.seekg(0);

    std::vector<char> compressed(fileSize);
    in.read(compressed.data(), fileSize);
    in.close();

    size_t decompressedSize = ZSTD_getFrameContentSize(compressed.data(), fileSize);
    if (ZSTD_isError(decompressedSize)) return false;

    outData.resize(decompressedSize);
    size_t result = ZSTD_decompress(outData.data(), decompressedSize, compressed.data(), fileSize);
    if (ZSTD_isError(result)) return false;

    return true;
}

void Server::saveChunkToFile(const std::shared_ptr<Chunk>& chunk) {
    std::ostringstream oss;
    const ChunkPosition& pos = chunk->getPosition();
    if (!std::filesystem::exists(std::filesystem::current_path().parent_path().parent_path() / "saves" / "world" / "chunks")) {
        std::filesystem::create_directories(std::filesystem::current_path().parent_path().parent_path() / "saves" / "world" / "chunks");
    }
    oss << std::filesystem::current_path().parent_path().parent_path().string() << "/saves/world/chunks/" << pos.x << "_" << pos.y << "_" << pos.z << ".zst";
    std::string filename = oss.str();

    std::vector<uint8_t> rawData;
    serializeChunk(chunk, rawData);

    size_t maxSize = ZSTD_compressBound(rawData.size());
    std::vector<char> compressed(maxSize);
    size_t compressedSize = ZSTD_compress(compressed.data(), maxSize, rawData.data(), rawData.size(), 1);

    if (ZSTD_isError(compressedSize)) {
        std::cerr << "[Server] ZSTD compression failed: " << ZSTD_getErrorName(compressedSize) << "\n";
        return;
    }

    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
        return;
    }

    out.write(compressed.data(), compressedSize);
    out.close();
}

std::shared_ptr<Chunk> Server::deserializeChunk(const std::vector<uint8_t>& in) {
    size_t offset = 0;

    int x = Serializer::readInt32(in, offset);
    int y = Serializer::readInt32(in, offset);
    int z = Serializer::readInt32(in, offset);
    ChunkPosition pos{x, y, z};

    auto chunk = std::make_shared<Chunk>();
    chunk->setPosition(pos);

    int blockCount = Serializer::readInt32(in, offset);
    std::array<uint16_t, CHUNK_VOLUME> blocks{};
    std::memcpy(blocks.data(), in.data() + offset, blockCount * sizeof(uint16_t));
    offset += blockCount * sizeof(uint16_t);
    chunk->setBlocks(blocks);

    int vertCount = Serializer::readInt32(in, offset);
    chunk->mesh.vertices.resize(vertCount);
    std::memcpy(chunk->mesh.vertices.data(), in.data() + offset, vertCount * sizeof(Vertex));
    offset += vertCount * sizeof(Vertex);

    int indexCount = Serializer::readInt32(in, offset);
    chunk->mesh.indices.resize(indexCount);
    std::memcpy(chunk->mesh.indices.data(), in.data() + offset, indexCount * sizeof(GLuint));
    offset += indexCount * sizeof(GLuint);

    chunk->mesh.isEmpty = chunk->mesh.vertices.empty() && chunk->mesh.indices.empty();
    chunk->mesh.needsUpdate = false;
    chunk->mesh.isUploaded = false;

    return chunk;
}

void Server::serializeChunk(const std::shared_ptr<Chunk>& chunk, std::vector<uint8_t>& out) {
    out.clear();
    Serializer::writeInt32(out, chunk->getPosition().x);
    Serializer::writeInt32(out, chunk->getPosition().y);
    Serializer::writeInt32(out, chunk->getPosition().z);

    const auto& blocks = chunk->getBlocks();
    Serializer::writeInt32(out, static_cast<int32_t>(blocks.size()));
    out.insert(out.end(),
        reinterpret_cast<const uint8_t*>(blocks.data()),
        reinterpret_cast<const uint8_t*>(blocks.data()) + blocks.size() * sizeof(uint16_t));

    const auto& verts = chunk->mesh.vertices;
    Serializer::writeInt32(out, static_cast<int32_t>(verts.size()));
    out.insert(out.end(),
        reinterpret_cast<const uint8_t*>(verts.data()),
        reinterpret_cast<const uint8_t*>(verts.data()) + verts.size() * sizeof(Vertex));

    const auto& indices = chunk->mesh.indices;
    Serializer::writeInt32(out, static_cast<int32_t>(indices.size()));
    out.insert(out.end(),
        reinterpret_cast<const uint8_t*>(indices.data()),
        reinterpret_cast<const uint8_t*>(indices.data()) + indices.size() * sizeof(GLuint));
}
