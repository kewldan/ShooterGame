#pragma once

#include "Protocol.h"
#include "UdpSocket.h"
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <vector>

namespace net {
    // A reliable event as it arrives (already deduplicated and in order).
    struct ReceivedEvent {
        EventType type;
        std::vector<uint8_t> payload;
    };

    // One end of a link to one peer: sequence numbers and a 33-packet ack window (Gaffer on Games
    // style, piggybacked on every packet), a reliable channel on top of it (events are re-sent every
    // RESEND_INTERVAL until a packet that carried them is acknowledged, and delivered exactly once, in
    // order, on the other side), a round-trip estimate from the acks, heartbeats and traffic counters.
    // Time is whatever clock the caller passes in (seconds).
    class Connection {
    public:
        struct Stats {
            float rtt = -1.f;             // seconds, smoothed; < 0 until the first ack
            int packetsInPerSecond = 0, packetsOutPerSecond = 0;
            float bytesInPerSecond = 0.f, bytesOutPerSecond = 0.f;
            unsigned packetsSent = 0, packetsLost = 0; // lost: never acknowledged
        };

    private:
        static constexpr size_t SENT_HISTORY = 256;   // packets remembered for acks (a power of two)
        static constexpr size_t MAX_PENDING = 512;     // unacknowledged reliable events before giving up
        static constexpr size_t MAX_OUT_OF_ORDER = 256; // buffered events waiting for a gap to close
        static constexpr float RTT_SMOOTHING = 0.1f;

        struct SentPacket {
            bool used = false, acked = false;
            uint16_t seq = 0;
            double time = 0.;
            std::vector<uint16_t> eventIds;
        };

        struct PendingEvent {
            uint16_t id;
            EventType type;
            std::vector<uint8_t> payload;
            double lastSent = -1e9;
        };

        Address remote;
        double lastReceiveTime, lastSendTime;
        bool receivedAnything = false;

        // Outgoing.
        uint16_t nextSeq = 0;
        SentPacket sent[SENT_HISTORY];
        std::deque<PendingEvent> pending;
        uint16_t nextEventId = 0;
        PacketType unreliableType = PacketType::None;
        std::vector<uint8_t> unreliable;

        // Incoming. Until the first packet arrives the ack field names a sequence number the peer
        // will not use for 65536 packets (it starts at 0), so that no packet is acknowledged by mistake.
        uint16_t remoteSeq = 0xFFFF; // newest sequence number seen
        uint32_t receivedBits = 0;   // the 32 before it
        uint16_t expectedEventId = 0;
        std::unordered_map<uint16_t, ReceivedEvent> outOfOrder;

        Stats stats;
        double statsWindowStart;
        int windowPacketsIn = 0, windowPacketsOut = 0;
        size_t windowBytesIn = 0, windowBytesOut = 0;

        void processAcks(uint16_t ack, uint32_t ackBits, double now);

        void noteReceivedSequence(uint16_t seq);

        void deliver(uint16_t id, ReceivedEvent &&event, std::vector<ReceivedEvent> &out);

        void sendPacket(UdpSocket &socket, PacketWriter &packet, SentPacket &record, double now);

    public:
        Connection(const Address &remote, double now);

        [[nodiscard]] const Address &getAddress() const { return remote; }

        // Queues an event for reliable delivery (the payload is copied).
        void sendReliable(EventType type, const PacketWriter &payload);

        // Sets the unreliable payload of the next packet (replacing one that was not sent yet).
        void setUnreliable(PacketType type, const PacketWriter &payload);

        // Sends what is due: the unreliable payload, reliable events not sent within RESEND_INTERVAL,
        // or a heartbeat when the link was silent for HEARTBEAT_INTERVAL. May send several packets.
        void flush(double now, UdpSocket &socket);

        // Parses one datagram from the peer. Returns false and changes nothing for anything malformed.
        // New reliable events come out in `events`; `unreliableType` is None when the packet carried no
        // unreliable payload, otherwise `unreliable` holds it.
        bool receive(double now, const uint8_t *data, size_t length, std::vector<ReceivedEvent> &events,
                     PacketType &unreliableType, std::vector<uint8_t> &unreliable);

        [[nodiscard]] bool isTimedOut(double now) const { return now - lastReceiveTime > CONNECTION_TIMEOUT; }

        // Too many events piled up without an acknowledgement: the peer is gone or hopelessly slow.
        [[nodiscard]] bool isCongested() const { return pending.size() >= MAX_PENDING; }

        [[nodiscard]] size_t getPendingCount() const { return pending.size(); }

        [[nodiscard]] const Stats &getStats() const { return stats; }

        // Rolls the per-second counters; call once per frame.
        void updateStats(double now);
    };
}
