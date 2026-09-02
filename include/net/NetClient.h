#pragma once

#include "Connection.h"
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace net {
    // What the server told the client, in arrival order (see NetClient::update).
    struct AcceptedEvent {
        AcceptMsg msg;
    };

    struct RejectedEvent {
        std::string reason;
    };

    struct DisconnectedEvent {
        std::string reason;
    };

    struct PlayerJoinedEvent {
        PlayerInfo info;
    };

    struct PlayerLeftEvent {
        uint8_t id;
    };

    struct SnapshotEvent {
        double receiveTime;
        Snapshot snapshot;
    };

    using ClientEvent = std::variant<AcceptedEvent, RejectedEvent, DisconnectedEvent, PlayerJoinedEvent,
            PlayerLeftEvent, ShotFxMsg, HitMsg, RespawnMsg, ChatMsg, SnapshotEvent>;

    // The client side of the link to the server: joins, streams the local state at STATE_INTERVAL,
    // sends shots and chat reliably and turns what comes back into ClientEvents.
    class NetClient {
    public:
        enum class Status { Connecting, Connected, Rejected, Disconnected };

    private:
        UdpSocket socket;
        Address server;
        std::unique_ptr<Connection> connection;
        Status status = Status::Connecting;
        uint8_t id = NO_PLAYER;
        double connectTime, lastStateTime = -1.;

        void disconnect(const std::string &reason, std::vector<ClientEvent> &events);

    public:
        NetClient(const Address &server, const std::string &name, double now);

        ~NetClient();

        NetClient(const NetClient &) = delete;

        NetClient &operator=(const NetClient &) = delete;

        [[nodiscard]] bool isOpen() const { return socket.isOpen(); }

        [[nodiscard]] uint16_t getLocalPort() const { return socket.getLocalPort(); }

        [[nodiscard]] Status getStatus() const { return status; }

        [[nodiscard]] uint8_t getId() const { return id; }

        [[nodiscard]] const Address &getServerAddress() const { return server; }

        void setSimulation(float loss, double latencySeconds) { socket.setSimulation(loss, latencySeconds); }

        // Receives and parses everything that arrived; the outcome is appended to `events`.
        void update(double now, std::vector<ClientEvent> &events);

        // Sends the local state at STATE_INTERVAL and flushes resends and heartbeats. Every frame.
        void sendState(double now, const PlayerState &state, bool alive);

        void sendShot(const glm::vec3 &origin, const glm::vec3 &direction);

        void sendChat(const std::string &text);

        // Tells the server goodbye (best effort, one packet) and stops.
        void leave(double now);

        [[nodiscard]] const Connection::Stats &getStats() const { return connection->getStats(); }
    };
}
