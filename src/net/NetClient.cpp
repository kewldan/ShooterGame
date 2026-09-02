#include "net/NetClient.h"

#include <plog/Log.h>

namespace net {
    NetClient::NetClient(const Address &server, const std::string &name, double now)
            : socket(0), server(server), connection(std::make_unique<Connection>(server, now)), connectTime(now) {
        if (!socket.isOpen() || !server.isValid()) {
            status = Status::Disconnected;
            return;
        }
        PacketWriter w;
        JoinMsg{name}.write(w);
        connection->sendReliable(EventType::Join, w);
        connection->flush(now, socket);
        PLOGI << "Connecting to " << server.toString() << " as [" << name << "]";
    }

    NetClient::~NetClient() = default;

    void NetClient::disconnect(const std::string &reason, std::vector<ClientEvent> &events) {
        if (status == Status::Connected || status == Status::Connecting) {
            status = Status::Disconnected;
            events.push_back(DisconnectedEvent{reason});
            PLOGW << "Disconnected: " << reason;
        }
    }

    void NetClient::update(double now, std::vector<ClientEvent> &events) {
        if (status == Status::Disconnected || status == Status::Rejected) {
            return;
        }
        uint8_t buffer[MAX_PACKET_SIZE];
        Address from;
        while (const size_t length = socket.receive(buffer, sizeof(buffer), from)) {
            if (!(from == server)) {
                continue; // only the server talks to us
            }
            std::vector<ReceivedEvent> received;
            PacketType type;
            std::vector<uint8_t> payload;
            if (!connection->receive(now, buffer, length, received, type, payload)) {
                continue;
            }
            for (const ReceivedEvent &event: received) {
                PacketReader reader(event.payload.data(), event.payload.size());
                switch (event.type) {
                    case EventType::Accept: {
                        AcceptMsg msg;
                        if (msg.read(reader) && status == Status::Connecting) {
                            status = Status::Connected;
                            id = msg.id;
                            events.push_back(AcceptedEvent{std::move(msg)});
                        }
                        break;
                    }
                    case EventType::Reject: {
                        RejectMsg msg;
                        if (msg.read(reader)) {
                            status = Status::Rejected;
                            events.push_back(RejectedEvent{msg.reason});
                            return;
                        }
                        break;
                    }
                    case EventType::PlayerJoined: {
                        PlayerInfo info;
                        if (info.read(reader)) events.push_back(PlayerJoinedEvent{std::move(info)});
                        break;
                    }
                    case EventType::PlayerLeft: {
                        PlayerLeftMsg msg;
                        if (msg.read(reader)) events.push_back(PlayerLeftEvent{msg.id});
                        break;
                    }
                    case EventType::ShotFx: {
                        ShotFxMsg msg;
                        if (msg.read(reader)) events.push_back(msg);
                        break;
                    }
                    case EventType::Hit: {
                        HitMsg msg;
                        if (msg.read(reader)) events.push_back(msg);
                        break;
                    }
                    case EventType::Respawn: {
                        RespawnMsg msg;
                        if (msg.read(reader)) events.push_back(msg);
                        break;
                    }
                    case EventType::Chat: {
                        ChatMsg msg;
                        if (msg.read(reader)) events.push_back(std::move(msg));
                        break;
                    }
                    default:
                        break; // client-to-server types have no business here
                }
            }
            if (type == PacketType::Snapshot && status == Status::Connected) {
                PacketReader reader(payload.data(), payload.size());
                SnapshotEvent snapshot{now, {}};
                if (snapshot.snapshot.read(reader)) events.push_back(std::move(snapshot));
            }
        }

        if (status == Status::Connecting && now - connectTime > CONNECTION_TIMEOUT) {
            disconnect("no answer from " + server.toString(), events);
        } else if (status == Status::Connected && connection->isTimedOut(now)) {
            disconnect("the server stopped responding", events);
        } else if (connection->isCongested()) {
            disconnect("the server does not acknowledge", events);
        }
    }

    void NetClient::sendState(double now, const PlayerState &state, bool alive) {
        if (status == Status::Disconnected || status == Status::Rejected) {
            return;
        }
        if (status == Status::Connected && now - lastStateTime >= STATE_INTERVAL) {
            lastStateTime = now;
            PacketWriter w;
            WirePlayerState::fromState(state, alive).write(w);
            connection->setUnreliable(PacketType::ClientState, w);
        }
        connection->flush(now, socket);
        connection->updateStats(now);
    }

    void NetClient::sendShot(const glm::vec3 &origin, const glm::vec3 &direction) {
        if (status != Status::Connected) return;
        PacketWriter w;
        ShotMsg{origin, direction}.write(w);
        connection->sendReliable(EventType::Shot, w);
    }

    void NetClient::sendChat(const std::string &text) {
        if (status != Status::Connected || text.empty()) return;
        PacketWriter w;
        ChatMsg{NO_PLAYER, text}.write(w);
        connection->sendReliable(EventType::Chat, w);
    }

    void NetClient::leave(double now) {
        if (status != Status::Connected) return;
        PacketWriter w;
        connection->sendReliable(EventType::Leave, w);
        connection->flush(now, socket);
        status = Status::Disconnected;
    }
}
