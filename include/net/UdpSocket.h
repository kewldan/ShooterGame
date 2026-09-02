#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace net {
    // An IPv4 endpoint in host byte order.
    struct Address {
        uint32_t ip = 0;
        uint16_t port = 0;

        bool operator==(const Address &other) const { return ip == other.ip && port == other.port; }

        [[nodiscard]] bool isValid() const { return port != 0; }

        [[nodiscard]] std::string toString() const;

        // "ip[:port]" or "host[:port]" (resolved with getaddrinfo); `defaultPort` when none is given.
        // Returns an invalid address (port 0) on failure.
        static Address parse(const std::string &text, uint16_t defaultPort);
    };

    // A non-blocking IPv4 UDP socket. Winsock is started with the first socket and stopped with the
    // last one. Optionally simulates a bad link on the receive side (setSimulation): packets are
    // dropped with a given probability and the rest are held back for a fixed delay, which is enough
    // to demonstrate interpolation and the reliable channel on localhost.
    class UdpSocket {
        uintptr_t handle;
        bool winsockAcquired = false;
        uint16_t localPort = 0;
        float lossProbability = 0.f;
        double latency = 0.; // seconds

        struct Delayed {
            double dueTime;
            Address from;
            std::vector<uint8_t> data;
        };

        std::deque<Delayed> delayed;
        uint32_t rngState = 0x9E3779B9u;

        static double now();

    public:
        // Binds to `port` (0 = any free port); isOpen() tells whether it worked.
        explicit UdpSocket(uint16_t port);

        ~UdpSocket();

        UdpSocket(const UdpSocket &) = delete;

        UdpSocket &operator=(const UdpSocket &) = delete;

        [[nodiscard]] bool isOpen() const;

        [[nodiscard]] uint16_t getLocalPort() const { return localPort; }

        // Returns false when the datagram could not be handed to the OS (never blocks).
        bool send(const Address &to, const void *data, size_t length);

        // Returns the size of the next datagram copied into `buffer` (at most `capacity` bytes), or 0
        // when nothing is waiting. Oversized datagrams are dropped.
        size_t receive(void *buffer, size_t capacity, Address &from);

        void setSimulation(float loss, double latencySeconds);
    };
}
