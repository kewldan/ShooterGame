#include "net/NetServer.h"

#include <algorithm>
#include <cmath>
#include <plog/Log.h>

namespace net {
    namespace {
        constexpr uint32_t LOOPBACK_IP = 0x7F000001; // 127.0.0.1

        // Nearest intersection of the ray (`origin`, unit `direction`) with the capsule around the
        // segment [a, b] of radius `radius`: the cylinder around the axis and the two spherical caps.
        // Returns the distance along the ray (>= 0) and the surface normal at the hit.
        bool rayCapsule(const glm::vec3 &origin, const glm::vec3 &direction, const glm::vec3 &a, const glm::vec3 &b,
                        float radius, float &distance, glm::vec3 &normal) {
            bool found = false;
            distance = 1e30f;
            const auto consider = [&](float t, const glm::vec3 &axisPoint) {
                if (t >= 0.f && t < distance) {
                    distance = t;
                    normal = glm::normalize(origin + direction * t - axisPoint);
                    found = true;
                }
            };

            // Infinite cylinder: the part of the ray perpendicular to the axis, as a quadratic in t.
            const glm::vec3 axis = b - a;
            const float axisLength2 = glm::dot(axis, axis);
            if (axisLength2 > 1e-8f) {
                const glm::vec3 ao = origin - a;
                const glm::vec3 q = direction - axis * (glm::dot(axis, direction) / axisLength2);
                const glm::vec3 p = ao - axis * (glm::dot(axis, ao) / axisLength2);
                const float A = glm::dot(q, q), B = 2.f * glm::dot(q, p), C = glm::dot(p, p) - radius * radius;
                if (A > 1e-8f) {
                    const float discriminant = B * B - 4.f * A * C;
                    if (discriminant >= 0.f) {
                        const float t = (-B - std::sqrt(discriminant)) / (2.f * A);
                        const float s = glm::dot(axis, ao + direction * t) / axisLength2;
                        if (s >= 0.f && s <= 1.f) consider(t, a + axis * s);
                    }
                }
            }
            // The caps.
            for (const glm::vec3 &centre: {a, b}) {
                const glm::vec3 oc = origin - centre;
                const float B = 2.f * glm::dot(direction, oc), C = glm::dot(oc, oc) - radius * radius;
                const float discriminant = B * B - 4.f * C;
                if (discriminant >= 0.f) consider((-B - std::sqrt(discriminant)) * 0.5f, centre);
            }
            return found;
        }
    }

    NetServer::NetServer(uint16_t port, std::vector<glm::vec3> spawnPoints, WorldRaycast raycast, Logger log, double now)
            : socket(port), spawnPoints(std::move(spawnPoints)), raycast(std::move(raycast)), log(std::move(log)),
              startTime(now) {
        if (this->spawnPoints.empty()) {
            this->spawnPoints.push_back(glm::vec3(0.f));
        }
        if (socket.isOpen()) {
            PLOGI << "Listen server on UDP port " << socket.getLocalPort();
        }
    }

    NetServer::Slot *NetServer::findByAddress(const Address &address) {
        for (Slot &slot: slots) {
            if (slot.used && slot.connection->getAddress() == address) return &slot;
        }
        return nullptr;
    }

    int NetServer::getPlayerCount() const {
        return static_cast<int>(std::count_if(slots.begin(), slots.end(), [](const Slot &s) { return s.used; }));
    }

    PlayerInfo NetServer::describe(const Slot &slot) const {
        PlayerInfo info;
        info.id = slot.id;
        info.name = slot.name;
        info.health = static_cast<uint8_t>(slot.health);
        info.kills = slot.kills;
        info.deaths = slot.deaths;
        const float rtt = slot.connection->getStats().rtt;
        info.ping = static_cast<uint16_t>(std::clamp(rtt < 0.f ? 0.f : rtt * 1000.f, 0.f, 65535.f));
        return info;
    }

    void NetServer::broadcast(EventType type, const PacketWriter &payload, uint8_t except) {
        for (Slot &slot: slots) {
            if (slot.used && slot.id != except) slot.connection->sendReliable(type, payload);
        }
    }

    void NetServer::handleNewPeer(const Address &from, const uint8_t *data, size_t length, double now) {
        // Parse with a throw-away connection: it becomes the real one only for a valid Join.
        auto probe = std::make_unique<Connection>(from, now);
        std::vector<ReceivedEvent> events;
        PacketType type;
        std::vector<uint8_t> payload;
        if (!probe->receive(now, data, length, events, type, payload)) {
            return;
        }
        const auto joinEvent = std::find_if(events.begin(), events.end(),
                                            [](const ReceivedEvent &e) { return e.type == EventType::Join; });
        if (joinEvent == events.end()) {
            return; // a stray packet of a session that is over
        }
        JoinMsg join;
        PacketReader reader(joinEvent->payload.data(), joinEvent->payload.size());
        if (!join.read(reader)) {
            return;
        }

        const auto free = std::find_if(slots.begin(), slots.end(), [](const Slot &s) { return !s.used; });
        if (free == slots.end()) {
            PacketWriter w;
            RejectMsg{"Server is full"}.write(w);
            probe->sendReliable(EventType::Reject, w);
            probe->flush(now, socket); // once; the client also gives up on its own
            PLOGW << "Rejected " << from.toString() << ": full";
            return;
        }
        Slot &slot = *free;
        slot = Slot{};
        slot.used = true;
        slot.id = static_cast<uint8_t>(free - slots.begin());
        slot.name = join.name.empty() ? "Player " + std::to_string(slot.id) : join.name;
        slot.isHost = from.ip == LOOPBACK_IP && from.port == hostPort;
        slot.connection = std::move(probe);

        AcceptMsg accept;
        accept.id = slot.id;
        for (const Slot &s: slots) {
            if (s.used) accept.players.push_back(describe(s));
        }
        PacketWriter w;
        accept.write(w);
        slot.connection->sendReliable(EventType::Accept, w);
        w.clear();
        describe(slot).write(w);
        broadcast(EventType::PlayerJoined, w, slot.id);
        log(slot.name + " joined from " + from.toString() + " as #" + std::to_string(slot.id));

        // Whatever else came along in that first packet.
        for (const ReceivedEvent &event: events) {
            if (event.type != EventType::Join && slot.used) handleEvent(slot, event, now);
        }
    }

    void NetServer::handleEvent(Slot &slot, const ReceivedEvent &event, double now) {
        PacketReader reader(event.payload.data(), event.payload.size());
        switch (event.type) {
            case EventType::Join: {
                // The client is still waiting for its Accept (a re-sent Join, or the first one crossed
                // with a lost Accept): tell it again. A connected client ignores the repeat.
                AcceptMsg accept;
                accept.id = slot.id;
                for (const Slot &s: slots) {
                    if (s.used) accept.players.push_back(describe(s));
                }
                PacketWriter w;
                accept.write(w);
                slot.connection->sendReliable(EventType::Accept, w);
                break;
            }
            case EventType::Leave:
                removePlayer(slot, "left");
                break;
            case EventType::Shot: {
                ShotMsg shot;
                if (shot.read(reader)) handleShot(slot, shot, now);
                break;
            }
            case EventType::Chat: {
                ChatMsg chat;
                if (chat.read(reader)) {
                    chat.sender = slot.id;
                    PacketWriter w;
                    chat.write(w);
                    broadcast(EventType::Chat, w);
                }
                break;
            }
            default:
                break; // a server-to-client type: nothing to do
        }
    }

    void NetServer::handleShot(Slot &shooter, const ShotMsg &shot, double now) {
        if (!shooter.alive || !shooter.hasState) {
            return;
        }
        const glm::vec3 eye = shooter.state.position + glm::vec3(0.f, Player::EYE_HEIGHT, 0.f);
        if (glm::distance(shot.origin, eye) > MAX_SHOT_ORIGIN_ERROR) {
            PLOGW << shooter.name << " shot from " << glm::distance(shot.origin, eye) << " units away from its body, ignored";
            return;
        }

        // The nearest other player along the ray...
        float bestDistance = Player::SHOT_RANGE;
        Slot *victim = nullptr;
        glm::vec3 victimNormal(0.f);
        const glm::vec3 halfAxis(0.f, Player::CAPSULE_HEIGHT * 0.5f, 0.f);
        for (Slot &other: slots) {
            if (!other.used || !other.hasState || !other.alive || &other == &shooter) continue;
            float distance;
            glm::vec3 normal;
            if (rayCapsule(shot.origin, shot.direction, other.state.position - halfAxis, other.state.position + halfAxis,
                           Player::CAPSULE_RADIUS, distance, normal) && distance < bestDistance) {
                bestDistance = distance;
                victim = &other;
                victimNormal = normal;
            }
        }
        // ...unless a wall or a crate is in the way. The host's own shot already pushed the crate locally.
        WorldHit world;
        if (raycast) {
            world = raycast(shot.origin, shot.direction, bestDistance, !shooter.isHost);
        }

        ShotFxMsg fx;
        fx.shooter = shooter.id;
        fx.origin = shot.origin;
        fx.direction = shot.direction;
        if (world.hit) {
            fx.kind = ShotFxMsg::World;
            fx.point = world.point;
            fx.normal = world.normal;
        } else if (victim) {
            fx.kind = ShotFxMsg::Player;
            fx.point = shot.origin + shot.direction * bestDistance;
            fx.normal = victimNormal;

            victim->health = std::max(victim->health - SHOT_DAMAGE, 0);
            HitMsg hit;
            hit.shooter = shooter.id;
            hit.victim = victim->id;
            hit.point = fx.point;
            hit.normal = victimNormal;
            hit.healthLeft = static_cast<uint8_t>(victim->health);
            PacketWriter w;
            hit.write(w);
            broadcast(EventType::Hit, w);
            if (victim->health == 0) {
                victim->alive = false;
                victim->deaths++;
                victim->respawnAt = now + RESPAWN_DELAY;
                shooter.kills++;
                log(shooter.name + " killed " + victim->name);
            }
        } else {
            fx.kind = ShotFxMsg::Miss;
            fx.point = shot.origin + shot.direction * Player::SHOT_RANGE;
        }
        PacketWriter w;
        fx.write(w);
        broadcast(EventType::ShotFx, w, shooter.id);
    }

    void NetServer::respawn(Slot &slot) {
        slot.health = MAX_HEALTH;
        slot.alive = true;
        const glm::vec3 eye = spawnPoints[static_cast<size_t>(nextSpawn++) % spawnPoints.size()];
        // Until the client reports from there, this is where the server assumes the player is.
        slot.state.position = eye - glm::vec3(0.f, Player::EYE_HEIGHT, 0.f);
        slot.state.velocity = glm::vec3(0.f);
        RespawnMsg msg;
        msg.id = slot.id;
        msg.position = eye;
        PacketWriter w;
        msg.write(w);
        broadcast(EventType::Respawn, w);
    }

    void NetServer::removePlayer(Slot &slot, const char *why) {
        log(slot.name + " " + why);
        const uint8_t id = slot.id;
        slot.used = false;
        slot.connection.reset();
        PacketWriter w;
        PlayerLeftMsg{id}.write(w);
        broadcast(EventType::PlayerLeft, w);
    }

    void NetServer::sendSnapshot(double now) {
        lastSnapshotTime = now;
        Snapshot snapshot;
        snapshot.serverTime = static_cast<uint32_t>((now - startTime) * 1000.);
        for (const Slot &slot: slots) {
            if (!slot.used || !slot.hasState) continue;
            Snapshot::Entry entry;
            entry.id = slot.id;
            entry.state = slot.state;
            // Life and death are the server's call, whatever the client claims.
            entry.state.flags = static_cast<uint8_t>((entry.state.flags & ~WirePlayerState::Alive) |
                                                     (slot.alive ? WirePlayerState::Alive : 0));
            entry.health = static_cast<uint8_t>(slot.health);
            entry.kills = slot.kills;
            entry.deaths = slot.deaths;
            entry.ping = describe(slot).ping;
            snapshot.players.push_back(entry);
        }
        snapshot.crates.assign(crates.begin(), crates.begin() + std::min(crates.size(), static_cast<size_t>(MAX_CRATES)));
        PacketWriter w;
        snapshot.write(w);
        if (!w.ok()) {
            PLOGE << "Snapshot does not fit into a packet";
            return;
        }
        for (Slot &slot: slots) {
            if (slot.used) slot.connection->setUnreliable(PacketType::Snapshot, w);
        }
    }

    void NetServer::update(double now) {
        if (!socket.isOpen()) {
            return;
        }
        uint8_t buffer[MAX_PACKET_SIZE];
        Address from;
        while (const size_t length = socket.receive(buffer, sizeof(buffer), from)) {
            Slot *slot = findByAddress(from);
            if (!slot) {
                handleNewPeer(from, buffer, length, now);
                continue;
            }
            std::vector<ReceivedEvent> events;
            PacketType type;
            std::vector<uint8_t> payload;
            if (!slot->connection->receive(now, buffer, length, events, type, payload)) {
                continue;
            }
            for (const ReceivedEvent &event: events) {
                if (!slot->used) break; // it left
                handleEvent(*slot, event, now);
            }
            if (slot->used && type == PacketType::ClientState) {
                PacketReader reader(payload.data(), payload.size());
                WirePlayerState state;
                if (state.read(reader)) {
                    slot->state = state;
                    slot->hasState = true;
                }
            }
        }

        for (Slot &slot: slots) {
            if (!slot.used) continue;
            if (slot.connection->isTimedOut(now)) {
                removePlayer(slot, "timed out");
            } else if (slot.connection->isCongested()) {
                removePlayer(slot, "dropped (not acknowledging)");
            } else if (!slot.alive && now >= slot.respawnAt) {
                respawn(slot);
            }
        }

        if (now - lastSnapshotTime >= SNAPSHOT_INTERVAL) {
            sendSnapshot(now);
        }
        for (Slot &slot: slots) {
            if (!slot.used) continue;
            slot.connection->flush(now, socket);
            slot.connection->updateStats(now);
        }
    }
}
