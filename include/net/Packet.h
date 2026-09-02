#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

// Little-endian binary packet builder/parser with bounds checks. Neither ever touches memory outside
// its buffer: a writer that runs out of room sets `overflow` and drops the rest, a reader that runs
// out of bytes returns zeros and sets `failed`. Callers check ok() once at the end of a message.
namespace net {
    constexpr size_t MAX_PACKET_SIZE = 1200; // stays below any common MTU

    class PacketWriter {
        uint8_t buffer[MAX_PACKET_SIZE]{};
        size_t size = 0;
        bool overflow = false;

        bool reserve(size_t bytes);

    public:
        void clear();

        void writeU8(uint8_t v);

        void writeU16(uint16_t v);

        void writeU32(uint32_t v);

        void writeI32(int32_t v);

        void writeFloat(float v);

        void writeVec3(const glm::vec3 &v);

        void writeQuat(const glm::quat &q);

        // Length-prefixed (u8) UTF-8 bytes, truncated to `maxLength`.
        void writeString(const std::string &s, size_t maxLength);

        void writeBytes(const void *data, size_t length);

        // Patches a u8 written earlier (a count that is only known at the end).
        void patchU8(size_t offset, uint8_t v);

        [[nodiscard]] const uint8_t *data() const { return buffer; }

        [[nodiscard]] size_t getSize() const { return size; }

        [[nodiscard]] size_t remaining() const { return MAX_PACKET_SIZE - size; }

        [[nodiscard]] bool ok() const { return !overflow; }
    };

    class PacketReader {
        const uint8_t *buffer;
        size_t size, position = 0;
        bool failed = false;

        bool take(size_t bytes);

    public:
        PacketReader(const uint8_t *data, size_t length) : buffer(data), size(length) {}

        uint8_t readU8();

        uint16_t readU16();

        uint32_t readU32();

        int32_t readI32();

        // NaN/inf never come out of here: they are replaced by 0 and fail the reader.
        float readFloat();

        glm::vec3 readVec3();

        // Normalised on the way in, identity if degenerate.
        glm::quat readQuat();

        std::string readString(size_t maxLength);

        // Returns a view into the packet (`length` bytes), or nullptr and fails when out of range.
        const uint8_t *readBytes(size_t length);

        // Marks the message as malformed (semantic checks by the caller: ids, enums, ranges).
        void fail() { failed = true; }

        [[nodiscard]] bool ok() const { return !failed; }

        [[nodiscard]] size_t remaining() const { return size - position; }

        [[nodiscard]] size_t getPosition() const { return position; }
    };
}
