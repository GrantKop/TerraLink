#ifndef SERVER_H
#define SERVER_H

#include "network/Address.h"
#include "network/Message.h"
#include "network/UDPSocket.h"
#include "core/world/Chunk.h"
#include "core/threads/ThreadSafeQueue.h"
#include "network/TCPSocket.h"


struct ClientInfo {
    SOCKET TCP_socket;
    uint16_t TCP_port;
    uint16_t UDP_port;

    std::string ip;

    std::string clientName;
};

struct PendingRequest {
    ChunkPosition pos;
    Address client;
};

class Server {
public:
    Server(uint16_t listenPort);
    void run();

    void stop();

    void handlePendingRequests();

private:
    TCPSocket listener;
    UDPSocket socket;
    SOCKET clientSocket = INVALID_SOCKET;
    ThreadSafeQueue<PendingRequest> chunkRequestQueue;

    std::unordered_map<std::string, std::shared_ptr<ClientInfo>> connectedClients;
    std::mutex clientMapMutex;

    void handleTCPClient(SOCKET socket);

    void handleMessage(const Message& msg, const Address& from);

    std::string getChunkFilePath(const ChunkPosition& pos);
    void saveChunkToFile(const std::shared_ptr<Chunk>& chunk);
    void saveCompressedChunkToFile(const ChunkPosition& pos, const std::vector<uint8_t>& compressedData);

    std::shared_ptr<Chunk> deserializeChunk(const std::vector<uint8_t>& in);
    void serializeChunk(const std::shared_ptr<Chunk>& chunk, std::vector<uint8_t>& out);
};

#endif
