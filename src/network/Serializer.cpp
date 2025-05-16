#include "Network/Serializer.h"

void Serializer::writeUInt8(std::vector<uint8_t>& buf, uint8_t val) {
    buf.push_back(val);
}

void Serializer::writeInt32(std::vector<uint8_t>& buf, int32_t val) {
    for (int i = 0; i < 4; ++i) {
        buf.push_back((val >> (i * 8)) & 0xFF);
    }
}

void Serializer::writeFloat(std::vector<uint8_t>& buf, float val) {
    uint8_t bytes[sizeof(float)];
    std::memcpy(bytes, &val, sizeof(float));
    buf.insert(buf.end(), bytes, bytes + sizeof(float));
}

void Serializer::writeVec3(std::vector<uint8_t>& buf, const glm::vec3& vec) {
    writeFloat(buf, vec.x);
    writeFloat(buf, vec.y);
    writeFloat(buf, vec.z);
}

uint8_t Serializer::readUInt8(const std::vector<uint8_t>& buf, size_t& offset) {
    return buf[offset++];
}

int32_t Serializer::readInt32(const std::vector<uint8_t>& buf, size_t& offset) {
    int32_t val = 0;
    for (int i = 0; i < 4; ++i) {
        val |= (buf[offset++] << (i * 8));
    }
    return val;
}

float Serializer::readFloat(const std::vector<uint8_t>& buf, size_t& offset) {
    float val;
    std::memcpy(&val, buf.data() + offset, sizeof(float));
    offset += sizeof(float);
    return val;
}

glm::vec3 Serializer::readVec3(const std::vector<uint8_t>& buf, size_t& offset) {
    float x = readFloat(buf, offset);
    float y = readFloat(buf, offset);
    float z = readFloat(buf, offset);
    return glm::vec3(x, y, z);
}
