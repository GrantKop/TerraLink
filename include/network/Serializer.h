#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <vector>
#include <cstdint>
#include <cstring>
#include <glm/glm.hpp>
#include <string>

class Serializer {
public:

    static void writeUInt8(std::vector<uint8_t>& buf, uint8_t val);
    static void writeInt32(std::vector<uint8_t>& buf, int32_t val);
    static void writeFloat(std::vector<uint8_t>& buf, float val);
    static void writeVec3(std::vector<uint8_t>& buf, const glm::vec3& vec);
    static void writeString(std::vector<uint8_t>& buf, const std::string& str);

    static uint8_t readUInt8(const std::vector<uint8_t>& buf, size_t& offset);
    static int32_t readInt32(const std::vector<uint8_t>& buf, size_t& offset);
    static float readFloat(const std::vector<uint8_t>& buf, size_t& offset);
    static glm::vec3 readVec3(const std::vector<uint8_t>& buf, size_t& offset);
    static std::string readString(const std::vector<uint8_t>& buf, size_t& offset);
};

#endif
