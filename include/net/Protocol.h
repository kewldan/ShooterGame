#pragma once

#include "Packet.h"
#include "Player.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

// The wire protocol of the listen server. Every datagram starts with a Header, followed by any
// number of reliable events (resent until acknowledged, delivered in order, see Connection) and at
// most one unreliable payload (the client's own state or the server's snapshot of everybody).
//
//   magic u16 | version u8 | type u8 | seq u16 | ack u16 | ackBits u32   (12 bytes)
//   eventCount u8, then per event: id u16 | type u8 | length u16 | payload
//   [ unreliable payload of `type`, up to the end of the datagram ]
//
// Everything is little-endian; floats are IEEE binary32. All messages have a write() and a read();
// read() returns false for anything malformed (short, bad enum, bad id, non-finite float).
namespace net {
    constexpr uint16_t MAGIC = 0x5347; // "SG"
    constexpr uint8_t VERSION = 1;
    constexpr uint16_t DEFAULT_PORT = 23403;
    constexpr int MAX_PLAYERS = 8;
    constexpr int MAX_CRATES = 16;
    constexpr size_t MAX_NAME_LENGTH = 24;
    constexpr size_t MAX_CHAT_LENGTH = 200;
    constexpr size_t MAX_REASON_LENGTH = 64;
    constexpr uint8_t NO_PLAYER = 0xFF;

    // Timing (seconds unless noted).
    constexpr double STATE_INTERVAL = 1. / 30.;    // client -> server: own state
    constexpr double SNAPSHOT_INTERVAL = 1. / 20.; // server -> everybody: all states
    constexpr double HEARTBEAT_INTERVAL = 0.5;     // an empty packet when nothing else was sent
    constexpr double RESEND_INTERVAL = 0.1;        // unacknowledged reliable events
    constexpr double CONNECTION_TIMEOUT = 5.;      // no packet at all from the peer
    constexpr double INTERPOLATION_DELAY = 0.1;    // remote players are shown this far in the past
    constexpr double RESPAWN_DELAY = 3.;

    // Gameplay rules the server enforces.
    constexpr int MAX_HEALTH = 100;
    constexpr int SHOT_DAMAGE = 25;
    // A shot whose origin is further than this from the shooter's last known eyes is ignored
    // (a stale state during a lag spike is fine, a teleporting shooter is not).
    constexpr float MAX_SHOT_ORIGIN_ERROR = 4.f;

    // The unreliable payload a packet carries.
    enum class PacketType : uint8_t {
        None = 0,        // acks, heartbeat and reliable events only
        ClientState = 1, // client -> server: WirePlayerState of the sender
        Snapshot = 2,    // server -> client: Snapshot
        Count
    };

    enum class EventType : uint8_t {
        Join = 1,      // client -> server: JoinMsg
        Accept,        // server -> client: AcceptMsg
        Reject,        // server -> client: RejectMsg
        PlayerJoined,  // server -> all: PlayerInfo
        PlayerLeft,    // server -> all: PlayerLeftMsg
        Leave,         // client -> server: (empty)
        Shot,          // client -> server: ShotMsg
        ShotFx,        // server -> all but the shooter: ShotFxMsg (validated outcome, for effects)
        Hit,           // server -> all: HitMsg
        Respawn,       // server -> all: RespawnMsg
        Chat,          // client -> server and server -> all: ChatMsg
        Count
    };

    struct Header {
        static constexpr size_t SIZE = 12;

        PacketType type = PacketType::None;
        uint16_t seq = 0, ack = 0;
        uint32_t ackBits = 0;

        void write(PacketWriter &w) const;

        // False when the magic or the version do not match (a foreign or old datagram).
        bool read(PacketReader &r);
    };

    // The replicated part of PlayerState (see Player.h). 33 bytes.
    struct WirePlayerState {
        enum Flags : uint8_t { Grounded = 1, Aiming = 2, Reloading = 4, Alive = 8 };

        glm::vec3 position{0.f}, velocity{0.f};
        float yaw = 0.f, pitch = 0.f;
        uint8_t flags = Alive;

        static WirePlayerState fromState(const PlayerState &state, bool alive);

        [[nodiscard]] bool has(Flags flag) const { return (flags & flag) != 0; }

        void write(PacketWriter &w) const;

        bool read(PacketReader &r);
    };

    // What everybody knows about a player besides its state.
    struct PlayerInfo {
        uint8_t id = NO_PLAYER;
        std::string name;
        uint8_t health = MAX_HEALTH;
        uint16_t kills = 0, deaths = 0;
        uint16_t ping = 0; // round trip to the server, ms

        void write(PacketWriter &w) const;

        bool read(PacketReader &r);
    };

    struct JoinMsg {
        std::string name;

        void write(PacketWriter &w) const;

        bool read(PacketReader &r);
    };

    struct AcceptMsg {
        uint8_t id = NO_PLAYER;
        std::vector<PlayerInfo> players; // everybody already there, including the new player

        void write(PacketWriter &w) const;

        bool read(PacketReader &r);
    };

    struct RejectMsg {
        std::string reason;

        void write(PacketWriter &w) const;

        bool read(PacketReader &r);
    };

    struct PlayerLeftMsg {
        uint8_t id = NO_PLAYER;

        void write(PacketWriter &w) const;

        bool read(PacketReader &r);
    };

    struct ShotMsg {
        glm::vec3 origin{0.f}, direction{0.f};

        void write(PacketWriter &w) const;

        bool read(PacketReader &r);
    };

    // The server's verdict on a shot, for the effects on the other clients.
    struct ShotFxMsg {
        enum Kind : uint8_t { Miss = 0, World = 1, Player = 2 };

        uint8_t shooter = NO_PLAYER;
        uint8_t kind = Miss;
        glm::vec3 origin{0.f}, direction{0.f};
        glm::vec3 point{0.f}, normal{0.f}; // where the tracer ends (the decal spot for World)

        void write(PacketWriter &w) const;

        bool read(PacketReader &r);
    };

    struct HitMsg {
        uint8_t shooter = NO_PLAYER, victim = NO_PLAYER;
        glm::vec3 point{0.f}, normal{0.f};
        uint8_t healthLeft = 0; // 0: the victim died

        void write(PacketWriter &w) const;

        bool read(PacketReader &r);
    };

    struct RespawnMsg {
        uint8_t id = NO_PLAYER;
        glm::vec3 position{0.f}; // eye position

        void write(PacketWriter &w) const;

        bool read(PacketReader &r);
    };

    struct ChatMsg {
        uint8_t sender = NO_PLAYER; // ignored client -> server
        std::string text;

        void write(PacketWriter &w) const;

        bool read(PacketReader &r);
    };

    struct CrateState {
        glm::vec3 position{0.f};
        glm::quat rotation{1.f, 0.f, 0.f, 0.f};
    };

    // Everything that moves, 20 times a second.
    struct Snapshot {
        struct Entry {
            uint8_t id = NO_PLAYER;
            WirePlayerState state;
            uint8_t health = MAX_HEALTH;
            uint16_t kills = 0, deaths = 0, ping = 0;
        };

        uint32_t serverTime = 0; // ms
        std::vector<Entry> players;
        std::vector<CrateState> crates;

        void write(PacketWriter &w) const;

        bool read(PacketReader &r);
    };
}
