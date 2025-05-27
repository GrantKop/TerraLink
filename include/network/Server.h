#ifndef SERVER_H
#define SERVER_H

#include "network/Address.h"
#include "network/Message.h"
#include "network/UDPSocket.h"
#include "core/world/Chunk.h"
#include "core/threads/ThreadSafeQueue.h"
#include "network/TCPSocket.h"

struct PendingRequest {
    ChunkPosition pos;
    Address client;
};

enum class FileTaskType { Load, Save };

struct FileTask {
    FileTaskType type;
    ChunkPosition pos;
    std::shared_ptr<Chunk> chunkToSave;
    SOCKET clientSocket;
};

class Server {
public:
    Server(uint16_t listenPort);
    void run();

    void handlePendingRequests();

private:
    TCPSocket listener;
    UDPSocket socket;
    SOCKET clientSocket = INVALID_SOCKET;
    ThreadSafeQueue<PendingRequest> chunkRequestQueue;

    void handleTCPClient(SOCKET socket);

    void handleMessage(const Message& msg, const Address& from);

    bool loadChunkFromFile(const ChunkPosition& pos, std::vector<uint8_t>& outData);
    void saveChunkToFile(const std::shared_ptr<Chunk>& chunk);

    std::shared_ptr<Chunk> deserializeChunk(const std::vector<uint8_t>& in);
    void serializeChunk(const std::shared_ptr<Chunk>& chunk, std::vector<uint8_t>& out);
};

#endif
