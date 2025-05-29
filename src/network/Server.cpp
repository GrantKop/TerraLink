#include "network/Server.h"
#include "network/Message.h"
#include "network/UDPSocket.h"
#include "network/Serializer.h"

#include <iostream>
#include <zstd.h>
#include <zstd_errors.h>
#include <Ws2tcpip.h> 

#include <csignal>

std::atomic<bool> serverRunning = true;

void signalHandler(int signum) {
    if (signum == SIGINT) {
        std::cout << "Caught signal " << signum << ", use 'stop' command to shut down server\n";
    }
    serverRunning = false;
}

Server::Server(uint16_t listenPort) {
    if (!socket.bind(listenPort)) {
        std::cerr << "[Server] Failed to bind UDP socket on port " << listenPort << std::endl;
        std::exit(1);
    }

    if (!listener.bind(listenPort)) {
        std::cerr << "[Server] Failed to bind TCP socket on port " << listenPort << std::endl;
        std::exit(1);
    }

    if (!listener.listen()) {
        std::cerr << "[Server] Failed to listen on TCP socket\n";
        std::exit(1);
    }

    std::cout << "[Server] Listening on port " << listenPort << "\n";

}

void Server::run() {
    std::signal(SIGINT, signalHandler);

    std::thread([this]() {
        while (serverRunning) {
            handlePendingRequests();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }).detach();

    std::thread([this]() {
        while (serverRunning) {
            sockaddr_in clientAddr;
            socklen_t addrLen = sizeof(clientAddr);
            SOCKET clientSocket = ::accept(listener.getHandle(), (sockaddr*)&clientAddr, &addrLen);
            if (clientSocket != INVALID_SOCKET) {
                char ipStr[INET_ADDRSTRLEN];
            #ifdef _WIN32
                InetNtopA(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
            #else
                inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
            #endif
                uint16_t port = ntohs(clientAddr.sin_port);

                Address addr;
                addr.ip = ipStr;
                addr.port = port;

                {
                    std::lock_guard<std::mutex> lock(clientMapMutex);
                    tcpClients[clientSocket] = addr;
                }
            
                std::cout << "[Server] New client connected: " << addr.ip << ":" << addr.port << "\n";
                std::thread(&Server::handleTCPClient, this, clientSocket).detach();
            }
        }
    }).detach();

    while (serverRunning) {
        std::vector<uint8_t> buffer;
        Address from;

        if (socket.receiveFrom(buffer, from)) {
            try {
                Message msg = Message::deserialize(buffer);
                handleMessage(msg, from);
            } catch (...) {
                std::cerr << "[Server] Failed to deserialize incoming UDP message\n";
            }
        }
    }

    std::cout << "[Server] Shutting down.\n";
    stop();
}

void Server::stop() {
    serverRunning = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    socket.bind(0);
    socket.close();
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
                if (true) { // Replace with actual file existence check !!!!!!!!!!!!!!!!!!¡
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
    {
        std::lock_guard<std::mutex> lock(clientMapMutex);
        tcpClients.erase(socket);
    }
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
        size_t offset = 0;
        int32_t x = Serializer::readInt32(msg.data, offset);
        int32_t y = Serializer::readInt32(msg.data, offset);
        int32_t z = Serializer::readInt32(msg.data, offset);
        ChunkPosition pos{x, y, z};

        int32_t compressedSize = Serializer::readInt32(msg.data, offset);
        if (msg.data.size() - offset < static_cast<size_t>(compressedSize)) {
            std::cerr << "[Server] Payload shorter than expected compressed size\n";
            return;
        }

        std::vector<uint8_t> compressedChunk(msg.data.begin() + offset,
                                             msg.data.begin() + offset + compressedSize);

        saveCompressedChunkToFile(pos, compressedChunk);
    }
    else if (msg.type == MessageType::ClientChunkUpdate) {
        std::cout << "[Server] Received chunk update from client\n";
        size_t offset = 0;
        int32_t x = Serializer::readInt32(msg.data, offset);
        int32_t y = Serializer::readInt32(msg.data, offset);
        int32_t z = Serializer::readInt32(msg.data, offset);
        ChunkPosition pos{x, y, z};

        int32_t compressedSize = Serializer::readInt32(msg.data, offset);
        if (msg.data.size() - offset < static_cast<size_t>(compressedSize)) {
            std::cerr << "[Server] Payload shorter than expected compressed size\n";
            return;
        }

        std::vector<uint8_t> compressedChunk(
            msg.data.begin() + offset,
            msg.data.begin() + offset + compressedSize
        );

        saveCompressedChunkToFile(pos, compressedChunk);

        std::vector<uint8_t> serialized = msg.serialize();
        uint32_t len = static_cast<uint32_t>(serialized.size());
        std::vector<uint8_t> lengthPrefix(4);
        std::memcpy(lengthPrefix.data(), &len, 4);
        
        std::lock_guard<std::mutex> lock(clientMapMutex);
        for (const auto& [sock, addr] : tcpClients) {
            if (addr.ip == from.ip && addr.port == from.port) continue;
        
            TCPSocket::sendAll(sock, lengthPrefix);
            TCPSocket::sendAll(sock, serialized);
        }
    }

    else if (msg.type == MessageType::PingPong) {
        Message pong;
        pong.type = MessageType::PingPong;
        socket.sendTo(pong.serialize(), from);
    }
}

void Server::handlePendingRequests() {
    PendingRequest req;
    while (chunkRequestQueue.tryPop(req)) {
        std::filesystem::path filePath = getChunkFilePath(req.pos);
        if (!std::filesystem::exists(filePath)) {
            Message notFound;
            notFound.type = MessageType::ChunkNotFound;
            Serializer::writeInt32(notFound.data, req.pos.x);
            Serializer::writeInt32(notFound.data, req.pos.y);
            Serializer::writeInt32(notFound.data, req.pos.z);
            socket.sendTo(notFound.serialize(), req.client);
            continue;
        }

        std::ifstream in(filePath, std::ios::binary | std::ios::ate);
        if (!in.is_open()) {
            std::cerr << "[Server] Failed to open chunk file\n";
            continue;
        }

        std::streamsize fileSize = in.tellg();
        in.seekg(0);

        std::vector<char> compressed(fileSize);
        in.read(compressed.data(), fileSize);
        in.close();

        Message message;
        message.type = MessageType::ChunkData;

        Serializer::writeInt32(message.data, req.pos.x);
        Serializer::writeInt32(message.data, req.pos.y);
        Serializer::writeInt32(message.data, req.pos.z);
        Serializer::writeInt32(message.data, static_cast<int32_t>(fileSize));

        message.data.insert(message.data.end(),
                            reinterpret_cast<uint8_t*>(compressed.data()),
                            reinterpret_cast<uint8_t*>(compressed.data()) + fileSize);

        std::vector<uint8_t> packet = message.serialize();
        if (packet.size() > 65507) {
            std::cerr << "[Server] Chunk too large to send over UDP: " << packet.size() << " bytes\n";
            continue;
        }

        socket.sendTo(packet, req.client);
    }
}

std::string Server::getChunkFilePath(const ChunkPosition& pos) {
    std::ostringstream oss;
    oss << std::filesystem::current_path().parent_path().parent_path().string()
        << "/saves/world/chunks/" << pos.x << "_" << pos.y << "_" << pos.z << ".zst";
    return oss.str();
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

void Server::saveCompressedChunkToFile(const ChunkPosition& pos, const std::vector<uint8_t>& compressedData) {
    std::filesystem::path dir = std::filesystem::current_path().parent_path().parent_path() / "saves" / "world" / "chunks";
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }

    std::ostringstream oss;
    oss << dir.string() << "/" << pos.x << "_" << pos.y << "_" << pos.z << ".zst";
    std::string filename = oss.str();

    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "[Server] Failed to open file for writing: " << filename << "\n";
        return;
    }

    out.write(reinterpret_cast<const char*>(compressedData.data()), compressedData.size());
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
