#pragma once

#include "GameObject.h"
#include "Protocol.h"
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace net {
    // The other players as this client sees them, and the crates as the host simulates them.
    //
    // Remote players are shown `interpolationDelay` seconds in the past, between the two snapshots
    // around that moment (positions lerped, angles lerped the short way round), so that 20 snapshots
    // a second look like continuous motion; when the next snapshot is late they coast on their last
    // velocity for a short while. Each one has a kinematic capsule with the player model in the
    // physics world: it is drawn through the usual GameObject passes (geometry, shadows, minimap),
    // blocks the local player and stops local rays (so a tracer ends on the body).
    // Crates on a client are kinematic too and follow the host's transforms the same way.
    class Replication {
    public:
        struct RemotePlayer {
            struct Sample {
                double time; // local time the snapshot arrived
                WirePlayerState state;
            };

            uint8_t id = NO_PLAYER;
            std::string name;
            int health = MAX_HEALTH;
            uint16_t kills = 0, deaths = 0, ping = 0;
            bool alive = true;
            std::deque<Sample> samples;
            WirePlayerState current;  // as interpolated by update()
            bool hasCurrent = false;  // at least one snapshot mentioned this player
            bool extrapolating = false;
            std::unique_ptr<GameObject> body;

            [[nodiscard]] glm::vec3 getEyePosition() const;

            [[nodiscard]] glm::vec3 getForward() const;

            [[nodiscard]] glm::vec3 getRight() const;
        };

    private:
        static constexpr size_t MAX_SAMPLES = 32;

        struct CrateSample {
            double time;
            std::vector<CrateState> crates;
        };

        btDynamicsWorld *world;
        std::vector<std::unique_ptr<GameObject>> *crates; // null: the crates are simulated here (host)
        std::vector<std::unique_ptr<RemotePlayer>> players;
        std::deque<CrateSample> crateSamples;

        // Turns a body added as static into a kinematic one that can be moved with setWorldTransform.
        void makeKinematic(GameObject &object) const;

        static void placeBody(GameObject &object, const glm::vec3 &position, const glm::quat &rotation);

    public:
        float interpolationDelay = static_cast<float>(INTERPOLATION_DELAY); // seconds
        float maxExtrapolation = 0.25f;                                     // seconds

        // `crates` (may be null) are made kinematic and driven by the snapshots from then on.
        Replication(btDynamicsWorld *world, std::vector<std::unique_ptr<GameObject>> *crates);

        Replication(const Replication &) = delete;

        Replication &operator=(const Replication &) = delete;

        void addPlayer(const PlayerInfo &info);

        void removePlayer(uint8_t id);

        void clear();

        [[nodiscard]] RemotePlayer *find(uint8_t id);

        [[nodiscard]] const std::vector<std::unique_ptr<RemotePlayer>> &getPlayers() const { return players; }

        // Records a snapshot received at `now`; the entry of `localId` is skipped.
        void applySnapshot(double now, const Snapshot &snapshot, uint8_t localId);

        // Interpolates everybody to `now - interpolationDelay` and moves the bodies. Once per frame.
        void update(double now);

        void draw(Engine::Shader *shader, const Frustum *frustum, CullStats *stats);

        [[nodiscard]] bool isRemoteBody(const GameObject *object) const;

        // Names and health bars above the heads, projected with `viewProjection` onto the main viewport.
        void drawNameplates(const glm::mat4 &viewProjection) const;

        [[nodiscard]] int getExtrapolatingCount() const;
    };
}
