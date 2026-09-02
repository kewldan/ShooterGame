#include "net/Packet.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace net {
    bool PacketWriter::reserve(size_t bytes) {
        if (overflow || size + bytes > MAX_PACKET_SIZE) {
            overflow = true;
            return false;
        }
        return true;
    }

    void PacketWriter::clear() {
        size = 0;
        overflow = false;
    }

    void PacketWriter::writeU8(uint8_t v) {
        if (reserve(1)) buffer[size++] = v;
    }

    void PacketWriter::writeU16(uint16_t v) {
        if (!reserve(2)) return;
        buffer[size++] = static_cast<uint8_t>(v);
        buffer[size++] = static_cast<uint8_t>(v >> 8);
    }

    void PacketWriter::writeU32(uint32_t v) {
        if (!reserve(4)) return;
        for (int i = 0; i < 4; i++) buffer[size++] = static_cast<uint8_t>(v >> (8 * i));
    }

    void PacketWriter::writeI32(int32_t v) {
        writeU32(static_cast<uint32_t>(v));
    }

    void PacketWriter::writeFloat(float v) {
        uint32_t bits;
        static_assert(sizeof(bits) == sizeof(v));
        std::memcpy(&bits, &v, sizeof(bits));
        writeU32(bits);
    }

    void PacketWriter::writeVec3(const glm::vec3 &v) {
        writeFloat(v.x);
        writeFloat(v.y);
        writeFloat(v.z);
    }

    void PacketWriter::writeQuat(const glm::quat &q) {
        writeFloat(q.x);
        writeFloat(q.y);
        writeFloat(q.z);
        writeFloat(q.w);
    }

    void PacketWriter::writeString(const std::string &s, size_t maxLength) {
        const size_t length = std::min({s.size(), maxLength, static_cast<size_t>(255)});
        if (!reserve(1 + length)) return;
        buffer[size++] = static_cast<uint8_t>(length);
        std::memcpy(buffer + size, s.data(), length);
        size += length;
    }

    void PacketWriter::writeBytes(const void *data, size_t length) {
        if (!reserve(length)) return;
        if (length > 0) std::memcpy(buffer + size, data, length);
        size += length;
    }

    void PacketWriter::patchU8(size_t offset, uint8_t v) {
        if (offset < size) buffer[offset] = v;
    }

    bool PacketReader::take(size_t bytes) {
        if (failed || bytes > size - position) {
            failed = true;
            return false;
        }
        return true;
    }

    uint8_t PacketReader::readU8() {
        return take(1) ? buffer[position++] : 0;
    }

    uint16_t PacketReader::readU16() {
        if (!take(2)) return 0;
        const uint16_t v = static_cast<uint16_t>(buffer[position] | (buffer[position + 1] << 8));
        position += 2;
        return v;
    }

    uint32_t PacketReader::readU32() {
        if (!take(4)) return 0;
        uint32_t v = 0;
        for (int i = 0; i < 4; i++) v |= static_cast<uint32_t>(buffer[position + i]) << (8 * i);
        position += 4;
        return v;
    }

    int32_t PacketReader::readI32() {
        return static_cast<int32_t>(readU32());
    }

    float PacketReader::readFloat() {
        const uint32_t bits = readU32();
        float v;
        std::memcpy(&v, &bits, sizeof(v));
        if (!std::isfinite(v)) {
            failed = true;
            return 0.f;
        }
        return v;
    }

    glm::vec3 PacketReader::readVec3() {
        const float x = readFloat(), y = readFloat(), z = readFloat();
        return {x, y, z};
    }

    glm::quat PacketReader::readQuat() {
        const float x = readFloat(), y = readFloat(), z = readFloat(), w = readFloat();
        const glm::quat q(w, x, y, z);
        const float length = glm::length(q);
        if (!(length > 1e-4f)) {
            return glm::quat(1.f, 0.f, 0.f, 0.f);
        }
        return q / length;
    }

    std::string PacketReader::readString(size_t maxLength) {
        const size_t length = readU8();
        if (!take(length)) return {};
        std::string s(reinterpret_cast<const char *>(buffer + position), length);
        position += length;
        if (s.size() > maxLength) {
            failed = true;
            return {};
        }
        // Control characters have no place in names or chat lines (they would also confuse ImGui).
        for (char &c: s) {
            if (static_cast<unsigned char>(c) < 0x20) c = ' ';
        }
        return s;
    }

    const uint8_t *PacketReader::readBytes(size_t length) {
        if (!take(length)) return nullptr;
        const uint8_t *p = buffer + position;
        position += length;
        return p;
    }
}
