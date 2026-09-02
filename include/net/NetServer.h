#pragma once

#include "Connection.h"
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace net {
    // The listen server: lives in the host's process next to its own client. It hands out player ids,
    // relays everybody's state as snapshots (SNAPSHOT_INTERVAL), validates shots against the capsules
    // of the other players plus the host's world, keeps health, kills and deaths, respawns the dead and
    // relays chat. Movement stays client-authoritative: a client's state is taken as it comes.
    class NetServer {
    public:
        struct WorldHit {
            bool hit = false;
            float distance = 0.f;
            glm::vec3 point{0.f}, normal{0.f}; // the normal faces the shooter
        };

        // Raycast against the map and the crates (never the players) of the host's world, up to
        // `maxDistance`. With `applyImpulse` a dynamic body that was hit is pushed like Player::fire does.
        using WorldRaycast = std::function<WorldHit(const glm::vec3 &origin, const glm::vec3 &direction,
                                                    float maxDistance, bool applyImpulse)>;
        using Logger = std::function<void(const std::string &line)>;

        struct Slot {
            bool used = false;
            bool isHost = false; // the host's own client (its shots already pushed the crates locally)
            uint8_t id = NO_PLAYER;
            std::string name;
            std::unique_ptr<Connection> connection;
            WirePlayerState state;
            bool hasState = false; // at least one ClientState arrived
            int health = MAX_HEALTH;
            bool alive = true;
            double respawnAt = 0.;
            uint16_t kills = 0, deaths = 0;
        };

    private:
        UdpSocket socket;
        std::vector<glm::vec3> spawnPoints; // eye positions
        WorldRaycast raycast;
        Logger log;
        uint16_t hostPort = 0;
        std::array<Slot, MAX_PLAYERS> slots;
        std::vector<CrateState> crates;
        double startTime, lastSnapshotTime = -1.;
        int nextSpawn = 0;

        Slot *findByAddress(const Address &address);

        // A packet from an address without a slot: only a Join gets one.
        void handleNewPeer(const Address &from, const uint8_t *data, size_t length, double now);

        void handleEvent(Slot &slot, const ReceivedEvent &event, double now);

        void handleShot(Slot &shooter, const ShotMsg &shot, double now);

        void broadcast(EventType type, const PacketWriter &payload, uint8_t except = NO_PLAYER);

        void removePlayer(Slot &slot, const char *why);

        void respawn(Slot &slot);

        void sendSnapshot(double now);

        [[nodiscard]] PlayerInfo describe(const Slot &slot) const;

    public:
        // `spawnPoints` are eye positions the dead come back at (see Player::reset), in turn.
        NetServer(uint16_t port, std::vector<glm::vec3> spawnPoints, WorldRaycast raycast, Logger log, double now);

        NetServer(const NetServer &) = delete;

        NetServer &operator=(const NetServer &) = delete;

        [[nodiscard]] bool isOpen() const { return socket.isOpen(); }

        [[nodiscard]] uint16_t getPort() const { return socket.getLocalPort(); }

        // The client connecting from this port of the loopback interface is the host's own.
        void setHostPort(uint16_t port) { hostPort = port; }

        // The crates as the host simulates them; sent with the next snapshot.
        void setCrates(const std::vector<CrateState> &states) { crates = states; }

        // Receives, validates, times out, snapshots and flushes. Once per frame.
        void update(double now);

        [[nodiscard]] const std::array<Slot, MAX_PLAYERS> &getSlots() const { return slots; }

        [[nodiscard]] int getPlayerCount() const;
    };
}
