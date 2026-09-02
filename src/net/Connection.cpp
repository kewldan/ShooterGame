#include "net/Connection.h"

#include <algorithm>

namespace net {
    namespace {
        // `a` is newer than `b`, allowing for the 16-bit wrap-around.
        bool sequenceGreater(uint16_t a, uint16_t b) {
            return (a > b && a - b <= 32768) || (a < b && b - a > 32768);
        }

        constexpr size_t EVENT_HEADER_SIZE = 2 + 1 + 2; // id, type, length
    }

    Connection::Connection(const Address &remote, double now)
            : remote(remote), lastReceiveTime(now), lastSendTime(-1e9), statsWindowStart(now) {
    }

    void Connection::sendReliable(EventType type, const PacketWriter &payload) {
        PendingEvent event{nextEventId++, type, std::vector<uint8_t>(payload.data(), payload.data() + payload.getSize())};
        pending.push_back(std::move(event));
    }

    void Connection::setUnreliable(PacketType type, const PacketWriter &payload) {
        unreliableType = type;
        unreliable.assign(payload.data(), payload.data() + payload.getSize());
    }

    void Connection::sendPacket(UdpSocket &socket, PacketWriter &packet, SentPacket &record, double now) {
        if (!packet.ok()) {
            return; // cannot happen: the builder checks the space before every write
        }
        if (record.used && !record.acked) {
            stats.packetsLost++; // the slot is reused after 256 packets; an ack that late is as good as none
        }
        record.used = true;
        record.acked = false;
        record.time = now;
        socket.send(remote, packet.data(), packet.getSize());
        lastSendTime = now;
        stats.packetsSent++;
        windowPacketsOut++;
        windowBytesOut += packet.getSize();
    }

    void Connection::flush(double now, UdpSocket &socket) {
        const bool heartbeatDue = now - lastSendTime >= HEARTBEAT_INTERVAL;
        // Reliable events due for a (re)send, in order.
        std::vector<PendingEvent *> due;
        for (PendingEvent &event: pending) {
            if (now - event.lastSent >= RESEND_INTERVAL) due.push_back(&event);
        }
        if (due.empty() && unreliableType == PacketType::None && !heartbeatDue) {
            return;
        }

        size_t nextDue = 0;
        bool first = true;
        // Usually one packet; more when the due events do not fit into one.
        while (first || nextDue < due.size()) {
            first = false;
            PacketWriter packet;
            Header header;
            header.seq = nextSeq++;
            header.ack = remoteSeq;
            header.ackBits = receivedBits;
            header.type = unreliableType;
            header.write(packet);
            const size_t countOffset = packet.getSize();
            packet.writeU8(0);
            SentPacket &record = sent[header.seq % SENT_HISTORY];
            record.seq = header.seq;
            record.eventIds.clear();

            // Leave room for the unreliable payload, which goes into this packet only.
            const size_t reservedTail = unreliableType != PacketType::None ? unreliable.size() : 0;
            int count = 0;
            while (nextDue < due.size() && count < 255) {
                PendingEvent &event = *due[nextDue];
                if (packet.remaining() < EVENT_HEADER_SIZE + event.payload.size() + reservedTail) {
                    break;
                }
                packet.writeU16(event.id);
                packet.writeU8(static_cast<uint8_t>(event.type));
                packet.writeU16(static_cast<uint16_t>(event.payload.size()));
                packet.writeBytes(event.payload.data(), event.payload.size());
                record.eventIds.push_back(event.id);
                event.lastSent = now;
                count++;
                nextDue++;
            }
            packet.patchU8(countOffset, static_cast<uint8_t>(count));
            if (unreliableType != PacketType::None) {
                packet.writeBytes(unreliable.data(), unreliable.size());
                unreliableType = PacketType::None;
                unreliable.clear();
            }
            if (count == 0 && nextDue < due.size()) {
                // A single event larger than a packet: it can never be sent, drop it rather than spin.
                due[nextDue]->payload.clear();
                due[nextDue]->lastSent = 1e30;
                nextDue++;
            }
            sendPacket(socket, packet, record, now);
        }
        std::erase_if(pending, [](const PendingEvent &event) { return event.lastSent >= 1e30; });
    }

    void Connection::noteReceivedSequence(uint16_t seq) {
        if (!receivedAnything) {
            receivedAnything = true;
            remoteSeq = seq;
            receivedBits = 0;
            return;
        }
        if (sequenceGreater(seq, remoteSeq)) {
            const uint16_t shift = static_cast<uint16_t>(seq - remoteSeq);
            receivedBits = shift >= 32 ? 0 : (receivedBits << shift) | (1u << (shift - 1));
            remoteSeq = seq;
        } else if (seq != remoteSeq) {
            const uint16_t behind = static_cast<uint16_t>(remoteSeq - seq);
            if (behind <= 32) receivedBits |= 1u << (behind - 1);
        }
    }

    void Connection::processAcks(uint16_t ack, uint32_t ackBits, double now) {
        for (int i = 0; i < 33; i++) {
            const bool acked = i == 0 || ((ackBits >> (i - 1)) & 1u);
            if (!acked) continue;
            const uint16_t seq = static_cast<uint16_t>(ack - i);
            SentPacket &record = sent[seq % SENT_HISTORY];
            if (!record.used || record.seq != seq || record.acked) continue;
            record.acked = true;
            const float sample = static_cast<float>(now - record.time);
            stats.rtt = stats.rtt < 0.f ? sample : stats.rtt + (sample - stats.rtt) * RTT_SMOOTHING;
            for (const uint16_t id: record.eventIds) {
                std::erase_if(pending, [id](const PendingEvent &event) { return event.id == id; });
            }
            record.eventIds.clear();
        }
    }

    void Connection::deliver(uint16_t id, ReceivedEvent &&event, std::vector<ReceivedEvent> &out) {
        if (id == expectedEventId) {
            out.push_back(std::move(event));
            expectedEventId++;
            // The gap closed: everything buffered behind it follows.
            for (auto it = outOfOrder.find(expectedEventId); it != outOfOrder.end();
                 it = outOfOrder.find(expectedEventId)) {
                out.push_back(std::move(it->second));
                outOfOrder.erase(it);
                expectedEventId++;
            }
        } else if (sequenceGreater(id, expectedEventId)) {
            if (outOfOrder.size() < MAX_OUT_OF_ORDER && !outOfOrder.contains(id)) {
                outOfOrder.emplace(id, std::move(event));
            }
        }
        // Older ids are duplicates of what was delivered already.
    }

    bool Connection::receive(double now, const uint8_t *data, size_t length, std::vector<ReceivedEvent> &events,
                             PacketType &unreliableType, std::vector<uint8_t> &unreliablePayload) {
        PacketReader reader(data, length);
        Header header;
        if (!header.read(reader)) {
            return false;
        }
        // Parse everything first so that a malformed packet leaves no trace.
        struct Parsed {
            uint16_t id;
            ReceivedEvent event;
        };
        std::vector<Parsed> parsed;
        const size_t count = reader.readU8();
        for (size_t i = 0; i < count && reader.ok(); i++) {
            const uint16_t id = reader.readU16();
            const uint8_t type = reader.readU8();
            const size_t size = reader.readU16();
            const uint8_t *payload = reader.readBytes(size);
            if (type == 0 || type >= static_cast<uint8_t>(EventType::Count) || !payload) {
                return false;
            }
            parsed.push_back({id, {static_cast<EventType>(type), std::vector<uint8_t>(payload, payload + size)}});
        }
        if (!reader.ok()) {
            return false;
        }
        if (header.type != PacketType::None && reader.remaining() == 0) {
            return false; // claims a payload it does not carry
        }

        lastReceiveTime = now;
        windowPacketsIn++;
        windowBytesIn += length;
        noteReceivedSequence(header.seq);
        processAcks(header.ack, header.ackBits, now);
        for (Parsed &p: parsed) {
            deliver(p.id, std::move(p.event), events);
        }
        unreliableType = header.type;
        unreliablePayload.clear();
        if (header.type != PacketType::None) {
            const size_t size = reader.remaining();
            const uint8_t *payload = reader.readBytes(size);
            unreliablePayload.assign(payload, payload + size);
        }
        return true;
    }

    void Connection::updateStats(double now) {
        const double elapsed = now - statsWindowStart;
        if (elapsed < 1.) return;
        stats.packetsInPerSecond = static_cast<int>(windowPacketsIn / elapsed);
        stats.packetsOutPerSecond = static_cast<int>(windowPacketsOut / elapsed);
        stats.bytesInPerSecond = static_cast<float>(windowBytesIn / elapsed);
        stats.bytesOutPerSecond = static_cast<float>(windowBytesOut / elapsed);
        windowPacketsIn = windowPacketsOut = 0;
        windowBytesIn = windowBytesOut = 0;
        statsWindowStart = now;
    }
}
