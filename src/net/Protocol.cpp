#include "net/Protocol.h"

namespace net {
    namespace {
        // Reads a player id that must name a slot (NO_PLAYER is not allowed here).
        uint8_t readPlayerId(PacketReader &r) {
            const uint8_t id = r.readU8();
            if (id >= MAX_PLAYERS) r.fail();
            return id;
        }

        template<typename T>
        bool readList(PacketReader &r, std::vector<T> &out, size_t maxCount, bool (*readOne)(PacketReader &, T &)) {
            const size_t count = r.readU8();
            if (count > maxCount) {
                r.fail();
                return false;
            }
            out.clear();
            out.reserve(count);
            for (size_t i = 0; i < count && r.ok(); i++) {
                T item;
                if (!readOne(r, item)) return false;
                out.push_back(std::move(item));
            }
            return r.ok();
        }
    }

    void Header::write(PacketWriter &w) const {
        w.writeU16(MAGIC);
        w.writeU8(VERSION);
        w.writeU8(static_cast<uint8_t>(type));
        w.writeU16(seq);
        w.writeU16(ack);
        w.writeU32(ackBits);
    }

    bool Header::read(PacketReader &r) {
        if (r.readU16() != MAGIC || r.readU8() != VERSION) {
            r.fail();
            return false;
        }
        const uint8_t t = r.readU8();
        if (t >= static_cast<uint8_t>(PacketType::Count)) {
            r.fail();
            return false;
        }
        type = static_cast<PacketType>(t);
        seq = r.readU16();
        ack = r.readU16();
        ackBits = r.readU32();
        return r.ok();
    }

    WirePlayerState WirePlayerState::fromState(const PlayerState &state, bool alive) {
        WirePlayerState w;
        w.position = state.position;
        w.velocity = state.velocity;
        w.yaw = state.yaw;
        w.pitch = state.pitch;
        w.flags = static_cast<uint8_t>((state.grounded ? Grounded : 0) | (state.aiming ? Aiming : 0) |
                                       (state.reloading ? Reloading : 0) | (alive ? Alive : 0));
        return w;
    }

    void WirePlayerState::write(PacketWriter &w) const {
        w.writeVec3(position);
        w.writeVec3(velocity);
        w.writeFloat(yaw);
        w.writeFloat(pitch);
        w.writeU8(flags);
    }

    bool WirePlayerState::read(PacketReader &r) {
        position = r.readVec3();
        velocity = r.readVec3();
        yaw = r.readFloat();
        pitch = r.readFloat();
        flags = r.readU8();
        return r.ok();
    }

    void PlayerInfo::write(PacketWriter &w) const {
        w.writeU8(id);
        w.writeString(name, MAX_NAME_LENGTH);
        w.writeU8(health);
        w.writeU16(kills);
        w.writeU16(deaths);
        w.writeU16(ping);
    }

    bool PlayerInfo::read(PacketReader &r) {
        id = readPlayerId(r);
        name = r.readString(MAX_NAME_LENGTH);
        health = r.readU8();
        kills = r.readU16();
        deaths = r.readU16();
        ping = r.readU16();
        return r.ok();
    }

    void JoinMsg::write(PacketWriter &w) const {
        w.writeString(name, MAX_NAME_LENGTH);
    }

    bool JoinMsg::read(PacketReader &r) {
        name = r.readString(MAX_NAME_LENGTH);
        return r.ok();
    }

    void AcceptMsg::write(PacketWriter &w) const {
        w.writeU8(id);
        w.writeU8(static_cast<uint8_t>(players.size()));
        for (const PlayerInfo &info: players) info.write(w);
    }

    bool AcceptMsg::read(PacketReader &r) {
        id = readPlayerId(r);
        return readList<PlayerInfo>(r, players, MAX_PLAYERS, [](PacketReader &r, PlayerInfo &p) { return p.read(r); });
    }

    void RejectMsg::write(PacketWriter &w) const {
        w.writeString(reason, MAX_REASON_LENGTH);
    }

    bool RejectMsg::read(PacketReader &r) {
        reason = r.readString(MAX_REASON_LENGTH);
        return r.ok();
    }

    void PlayerLeftMsg::write(PacketWriter &w) const {
        w.writeU8(id);
    }

    bool PlayerLeftMsg::read(PacketReader &r) {
        id = readPlayerId(r);
        return r.ok();
    }

    void ShotMsg::write(PacketWriter &w) const {
        w.writeVec3(origin);
        w.writeVec3(direction);
    }

    bool ShotMsg::read(PacketReader &r) {
        origin = r.readVec3();
        direction = r.readVec3();
        const float length = glm::length(direction);
        if (!(length > 0.5f && length < 2.f)) {
            r.fail(); // a direction is a unit vector, give or take rounding
            return false;
        }
        direction /= length;
        return r.ok();
    }

    void ShotFxMsg::write(PacketWriter &w) const {
        w.writeU8(shooter);
        w.writeU8(kind);
        w.writeVec3(origin);
        w.writeVec3(direction);
        w.writeVec3(point);
        w.writeVec3(normal);
    }

    bool ShotFxMsg::read(PacketReader &r) {
        shooter = readPlayerId(r);
        kind = r.readU8();
        if (kind > Player) r.fail();
        origin = r.readVec3();
        direction = r.readVec3();
        point = r.readVec3();
        normal = r.readVec3();
        return r.ok();
    }

    void HitMsg::write(PacketWriter &w) const {
        w.writeU8(shooter);
        w.writeU8(victim);
        w.writeVec3(point);
        w.writeVec3(normal);
        w.writeU8(healthLeft);
    }

    bool HitMsg::read(PacketReader &r) {
        shooter = readPlayerId(r);
        victim = readPlayerId(r);
        point = r.readVec3();
        normal = r.readVec3();
        healthLeft = r.readU8();
        if (healthLeft > MAX_HEALTH) r.fail();
        return r.ok();
    }

    void RespawnMsg::write(PacketWriter &w) const {
        w.writeU8(id);
        w.writeVec3(position);
    }

    bool RespawnMsg::read(PacketReader &r) {
        id = readPlayerId(r);
        position = r.readVec3();
        return r.ok();
    }

    void ChatMsg::write(PacketWriter &w) const {
        w.writeU8(sender);
        w.writeString(text, MAX_CHAT_LENGTH);
    }

    bool ChatMsg::read(PacketReader &r) {
        sender = r.readU8(); // NO_PLAYER is legal (a server message)
        if (sender != NO_PLAYER && sender >= MAX_PLAYERS) r.fail();
        text = r.readString(MAX_CHAT_LENGTH);
        if (text.empty()) r.fail();
        return r.ok();
    }

    void Snapshot::write(PacketWriter &w) const {
        w.writeU32(serverTime);
        w.writeU8(static_cast<uint8_t>(players.size()));
        for (const Entry &e: players) {
            w.writeU8(e.id);
            e.state.write(w);
            w.writeU8(e.health);
            w.writeU16(e.kills);
            w.writeU16(e.deaths);
            w.writeU16(e.ping);
        }
        w.writeU8(static_cast<uint8_t>(crates.size()));
        for (const CrateState &c: crates) {
            w.writeVec3(c.position);
            w.writeQuat(c.rotation);
        }
    }

    bool Snapshot::read(PacketReader &r) {
        serverTime = r.readU32();
        if (!readList<Entry>(r, players, MAX_PLAYERS, [](PacketReader &r, Entry &e) {
            e.id = readPlayerId(r);
            e.state.read(r);
            e.health = r.readU8();
            if (e.health > MAX_HEALTH) r.fail();
            e.kills = r.readU16();
            e.deaths = r.readU16();
            e.ping = r.readU16();
            return r.ok();
        })) {
            return false;
        }
        return readList<CrateState>(r, crates, MAX_CRATES, [](PacketReader &r, CrateState &c) {
            c.position = r.readVec3();
            c.rotation = r.readQuat();
            return r.ok();
        });
    }
}
