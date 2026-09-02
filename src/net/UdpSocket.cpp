#include "net/UdpSocket.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

#include "net/Packet.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <plog/Log.h>

// Not in every SDK's headers (it lives in mstcpip.h when it is).
#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif

namespace net {
    namespace {
        // One WSAStartup for the process, released when the last socket goes away.
        int winsockUsers = 0;

        bool acquireWinsock() {
            if (winsockUsers == 0) {
                WSADATA data;
                if (const int error = WSAStartup(MAKEWORD(2, 2), &data); error != 0) {
                    PLOGE << "WSAStartup failed: " << error;
                    return false;
                }
            }
            winsockUsers++;
            return true;
        }

        void releaseWinsock() {
            if (winsockUsers > 0 && --winsockUsers == 0) {
                WSACleanup();
            }
        }

        sockaddr_in toSockaddr(const Address &address) {
            sockaddr_in sa{};
            sa.sin_family = AF_INET;
            sa.sin_port = htons(address.port);
            sa.sin_addr.s_addr = htonl(address.ip);
            return sa;
        }

        Address fromSockaddr(const sockaddr_in &sa) {
            return {ntohl(sa.sin_addr.s_addr), ntohs(sa.sin_port)};
        }
    }

    std::string Address::toString() const {
        return std::to_string(ip >> 24) + "." + std::to_string((ip >> 16) & 0xFF) + "." +
               std::to_string((ip >> 8) & 0xFF) + "." + std::to_string(ip & 0xFF) + ":" + std::to_string(port);
    }

    Address Address::parse(const std::string &text, uint16_t defaultPort) {
        std::string host = text;
        uint16_t port = defaultPort;
        if (const size_t colon = text.rfind(':'); colon != std::string::npos) {
            host = text.substr(0, colon);
            const int p = std::atoi(text.c_str() + colon + 1);
            if (p <= 0 || p > 65535) {
                PLOGE << "Bad port in address [" << text << "]";
                return {};
            }
            port = static_cast<uint16_t>(p);
        }
        if (host.empty()) host = "127.0.0.1";
        // getaddrinfo needs Winsock, and no socket may exist yet.
        if (!acquireWinsock()) return {};
        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        addrinfo *result = nullptr;
        Address address;
        if (getaddrinfo(host.c_str(), nullptr, &hints, &result) == 0 && result) {
            address = fromSockaddr(*reinterpret_cast<const sockaddr_in *>(result->ai_addr));
            address.port = port;
            freeaddrinfo(result);
        } else {
            PLOGE << "Cannot resolve [" << host << "]";
        }
        releaseWinsock();
        return address;
    }

    double UdpSocket::now() {
        using namespace std::chrono;
        return duration<double>(steady_clock::now().time_since_epoch()).count();
    }

    UdpSocket::UdpSocket(uint16_t port) : handle(INVALID_SOCKET) {
        winsockAcquired = acquireWinsock();
        if (!winsockAcquired) {
            return;
        }
        const SOCKET s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (s == INVALID_SOCKET) {
            PLOGE << "socket() failed: " << WSAGetLastError();
            return;
        }
        sockaddr_in local{};
        local.sin_family = AF_INET;
        local.sin_port = htons(port);
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(s, reinterpret_cast<sockaddr *>(&local), sizeof(local)) == SOCKET_ERROR) {
            PLOGE << "bind() to port " << port << " failed: " << WSAGetLastError();
            closesocket(s);
            return;
        }
        u_long nonBlocking = 1;
        if (ioctlsocket(s, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
            PLOGE << "ioctlsocket(FIONBIO) failed: " << WSAGetLastError();
            closesocket(s);
            return;
        }
        // Windows reports an ICMP "port unreachable" caused by an earlier send as an error of the next
        // recvfrom; the connection timeout handles dead peers, so switch that off.
        BOOL off = FALSE;
        DWORD bytes = 0;
        WSAIoctl(s, SIO_UDP_CONNRESET, &off, sizeof(off), nullptr, 0, &bytes, nullptr, nullptr);

        sockaddr_in bound{};
        int length = sizeof(bound);
        if (getsockname(s, reinterpret_cast<sockaddr *>(&bound), &length) == 0) {
            localPort = ntohs(bound.sin_port);
        }
        handle = s;
        PLOGI << "UDP socket bound to port " << localPort;
    }

    UdpSocket::~UdpSocket() {
        if (handle != INVALID_SOCKET) {
            closesocket(handle);
        }
        if (winsockAcquired) {
            releaseWinsock();
        }
    }

    bool UdpSocket::isOpen() const {
        return handle != INVALID_SOCKET;
    }

    bool UdpSocket::send(const Address &to, const void *data, size_t length) {
        if (handle == INVALID_SOCKET || length > MAX_PACKET_SIZE) {
            return false;
        }
        const sockaddr_in sa = toSockaddr(to);
        const int sent = sendto(handle, static_cast<const char *>(data), static_cast<int>(length), 0,
                                reinterpret_cast<const sockaddr *>(&sa), sizeof(sa));
        return sent == static_cast<int>(length);
    }

    size_t UdpSocket::receive(void *buffer, size_t capacity, Address &from) {
        if (handle == INVALID_SOCKET) {
            return 0;
        }
        const bool simulated = lossProbability > 0.f || latency > 0.;
        const double time = simulated ? now() : 0.;
        // Drain the OS queue: straight out when nothing is simulated, otherwise into the delay line.
        for (;;) {
            uint8_t scratch[MAX_PACKET_SIZE];
            sockaddr_in sa{};
            int length = sizeof(sa);
            const int received = recvfrom(handle, reinterpret_cast<char *>(scratch), sizeof(scratch), 0,
                                          reinterpret_cast<sockaddr *>(&sa), &length);
            if (received <= 0) {
                break; // WSAEWOULDBLOCK, a reset, or an empty datagram: nothing (more) to deliver
            }
            if (static_cast<size_t>(received) > capacity) {
                continue; // oversized: not one of ours
            }
            if (!simulated) {
                std::memcpy(buffer, scratch, static_cast<size_t>(received));
                from = fromSockaddr(sa);
                return static_cast<size_t>(received);
            }
            // xorshift32: a cheap, deterministic coin for the loss simulation.
            rngState ^= rngState << 13;
            rngState ^= rngState >> 17;
            rngState ^= rngState << 5;
            if (static_cast<float>(rngState & 0xFFFFFF) / static_cast<float>(0x1000000) < lossProbability) {
                continue;
            }
            delayed.push_back({time + latency, fromSockaddr(sa), std::vector<uint8_t>(scratch, scratch + received)});
        }
        if (!delayed.empty() && delayed.front().dueTime <= time) {
            const Delayed &next = delayed.front();
            const size_t length = std::min(next.data.size(), capacity);
            std::memcpy(buffer, next.data.data(), length);
            from = next.from;
            delayed.pop_front();
            return length;
        }
        return 0;
    }

    void UdpSocket::setSimulation(float loss, double latencySeconds) {
        lossProbability = std::clamp(loss, 0.f, 1.f);
        latency = std::max(latencySeconds, 0.);
        if (lossProbability > 0.f || latency > 0.) {
            PLOGW << "Simulating a bad link on receive: " << lossProbability * 100.f << "% loss, "
                  << latency * 1000. << " ms delay";
        }
    }
}
