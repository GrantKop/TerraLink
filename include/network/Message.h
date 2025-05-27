#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdint>
#include <vector>

enum class MessageType : uint8_t {
    PlayerPositionUpdate,
    ChunkRequest,
    ChunkData,
    ChunkGeneratedByClient,
    ChunkNotFound,
    Ping,
    Pong
};

struct Message {
    MessageType type;
    std::vector<uint8_t> data;

    std::vector<uint8_t> serialize() const;
    static Message deserialize(const std::vector<uint8_t>& buffer);
};

#endif
