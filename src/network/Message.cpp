#include "Network/Message.h"
#include <cstring>
#include <stdexcept>

std::vector<uint8_t> Message::serialize() const {
    std::vector<uint8_t> buffer;

    buffer.reserve(1 + data.size());

    buffer.push_back(static_cast<uint8_t>(type));

    buffer.insert(buffer.end(), data.begin(), data.end());

    return buffer;
}

Message Message::deserialize(const std::vector<uint8_t>& buffer) {
    if (buffer.empty()) {
        throw std::runtime_error("Deserialize error: empty buffer");
    }

    Message msg;
    msg.type = static_cast<MessageType>(buffer[0]);
    msg.data.assign(buffer.begin() + 1, buffer.end());
    return msg;
}
