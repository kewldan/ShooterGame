#include "net/Replication.h"

#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>

namespace net {
    namespace {
        // Interpolation factor of `time` between the two samples around it; -1 when `time` is past the
        // newest sample (extrapolate) and the samples to use in `a`/`b`.
        template<typename Sample>
        float locate(const std::deque<Sample> &samples, double time, const Sample *&a, const Sample *&b) {
            a = b = nullptr;
            if (samples.empty()) return 0.f;
            if (time <= samples.front().time) {
                a = b = &samples.front();
                return 0.f;
            }
            for (size_t i = 0; i + 1 < samples.size(); i++) {
                if (time >= samples[i].time && time < samples[i + 1].time) {
                    a = &samples[i];
                    b = &samples[i + 1];
                    const double span = b->time - a->time;
                    return span > 1e-6 ? static_cast<float>((time - a->time) / span) : 1.f;
                }
            }
            a = b = &samples.back();
            return -1.f;
        }

        float lerpAngle(float from, float to, float t) {
            float diff = std::fmod(to - from, glm::two_pi<float>());
            if (diff > glm::pi<float>()) diff -= glm::two_pi<float>();
            if (diff < -glm::pi<float>()) diff += glm::two_pi<float>();
            return from + diff * t;
        }

        glm::vec3 nameplateAnchor(const glm::vec3 &position) {
            return position + glm::vec3(0.f, Player::CAPSULE_HEIGHT * 0.5f + Player::CAPSULE_RADIUS + 0.4f, 0.f);
        }
    }

    glm::vec3 Replication::RemotePlayer::getEyePosition() const {
        return current.position + glm::vec3(0.f, Player::EYE_HEIGHT, 0.f);
    }

    glm::vec3 Replication::RemotePlayer::getForward() const {
        const float cosPitch = std::cos(current.pitch);
        return {std::sin(current.yaw) * cosPitch, -std::sin(current.pitch), -std::cos(current.yaw) * cosPitch};
    }

    glm::vec3 Replication::RemotePlayer::getRight() const {
        return {std::cos(current.yaw), 0.f, std::sin(current.yaw)};
    }

    Replication::Replication(btDynamicsWorld *world, std::vector<std::unique_ptr<GameObject>> *crates)
            : world(world), crates(crates) {
        if (crates) {
            for (auto &crate: *crates) makeKinematic(*crate);
        }
    }

    void Replication::makeKinematic(GameObject &object) const {
        btRigidBody &rb = *object.rb;
        world->removeRigidBody(&rb);
        rb.setMassProps(0.f, btVector3(0.f, 0.f, 0.f));
        rb.setCollisionFlags((rb.getCollisionFlags() & ~btCollisionObject::CF_STATIC_OBJECT) |
                             btCollisionObject::CF_KINEMATIC_OBJECT);
        rb.setActivationState(DISABLE_DEACTIVATION);
        rb.setLinearVelocity(btVector3(0.f, 0.f, 0.f));
        rb.setAngularVelocity(btVector3(0.f, 0.f, 0.f));
        world->addRigidBody(&rb);
    }

    void Replication::placeBody(GameObject &object, const glm::vec3 &position, const glm::quat &rotation) {
        btTransform transform;
        transform.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
        transform.setOrigin(btVector3(position.x, position.y, position.z));
        object.rb->setWorldTransform(transform);
        object.motionState->setWorldTransform(transform);
    }

    void Replication::addPlayer(const PlayerInfo &info) {
        if (info.id >= MAX_PLAYERS) return;
        RemotePlayer *player = find(info.id);
        if (!player) {
            players.push_back(std::make_unique<RemotePlayer>());
            player = players.back().get();
            player->id = info.id;
        }
        player->name = info.name;
        player->health = info.health;
        player->kills = info.kills;
        player->deaths = info.deaths;
        player->ping = info.ping;
        player->alive = info.health > 0;
    }

    void Replication::removePlayer(uint8_t id) {
        std::erase_if(players, [id](const std::unique_ptr<RemotePlayer> &p) { return p->id == id; });
    }

    void Replication::clear() {
        players.clear();
        crateSamples.clear();
    }

    Replication::RemotePlayer *Replication::find(uint8_t id) {
        for (auto &player: players) {
            if (player->id == id) return player.get();
        }
        return nullptr;
    }

    void Replication::applySnapshot(double now, const Snapshot &snapshot, uint8_t localId) {
        for (const Snapshot::Entry &entry: snapshot.players) {
            if (entry.id == localId) continue;
            RemotePlayer *player = find(entry.id);
            if (!player) continue; // not introduced yet (the Accept/PlayerJoined event is on its way)
            player->health = entry.health;
            player->kills = entry.kills;
            player->deaths = entry.deaths;
            player->ping = entry.ping;
            player->alive = entry.state.has(WirePlayerState::Alive);
            // Snapshots may arrive out of order: only newer ones extend the timeline.
            if (player->samples.empty() || now > player->samples.back().time) {
                player->samples.push_back({now, entry.state});
                if (player->samples.size() > MAX_SAMPLES) player->samples.pop_front();
            }
            if (!player->hasCurrent) {
                player->current = entry.state;
                player->hasCurrent = true;
            }
        }
        if (crates && !snapshot.crates.empty() && (crateSamples.empty() || now > crateSamples.back().time)) {
            crateSamples.push_back({now, snapshot.crates});
            if (crateSamples.size() > MAX_SAMPLES) crateSamples.pop_front();
        }
    }

    void Replication::update(double now) {
        const double renderTime = now - interpolationDelay;
        for (auto &player: players) {
            if (!player->hasCurrent) continue;
            const RemotePlayer::Sample *a, *b;
            const float t = locate(player->samples, renderTime, a, b);
            if (t < 0.f) {
                // Past the newest sample: coast along its velocity for a moment, then hold.
                const float ahead = std::min(static_cast<float>(renderTime - a->time), maxExtrapolation);
                player->current = a->state;
                player->current.position += a->state.velocity * ahead;
                player->extrapolating = ahead > 0.f;
            } else {
                player->current = b->state;
                player->current.position = glm::mix(a->state.position, b->state.position, t);
                player->current.velocity = glm::mix(a->state.velocity, b->state.velocity, t);
                player->current.yaw = lerpAngle(a->state.yaw, b->state.yaw, t);
                player->current.pitch = glm::mix(a->state.pitch, b->state.pitch, t);
                player->extrapolating = false;
            }
            if (!player->body) {
                // Created here, at a known position, so that it never sits at the origin in somebody's way.
                player->body = std::make_unique<GameObject>(world, "player.obj", 0.f,
                                                            new btCapsuleShape(Player::CAPSULE_RADIUS, Player::CAPSULE_HEIGHT));
                player->body->setCastShadows(true);
                makeKinematic(*player->body);
            }
            // The model faces -Z; Player::getForward() is (sin yaw, 0, -cos yaw), i.e. a turn of -yaw about Y.
            // A dead player is parked far below the map so that its invisible body blocks nothing.
            const glm::vec3 bodyPosition = player->alive ? player->current.position
                                                         : player->current.position - glm::vec3(0.f, 1000.f, 0.f);
            placeBody(*player->body, bodyPosition, glm::angleAxis(-player->current.yaw, glm::vec3(0.f, 1.f, 0.f)));
        }

        if (crates && !crateSamples.empty()) {
            const CrateSample *a, *b;
            float t = locate(crateSamples, renderTime, a, b);
            if (t < 0.f) t = 1.f; // hold the newest transforms
            const size_t count = std::min({crates->size(), a->crates.size(), b->crates.size()});
            for (size_t i = 0; i < count; i++) {
                const CrateState &from = a->crates[i], &to = b->crates[i];
                placeBody(*(*crates)[i], glm::mix(from.position, to.position, t), glm::slerp(from.rotation, to.rotation, t));
            }
        }
    }

    void Replication::draw(Engine::Shader *shader, const Frustum *frustum, CullStats *stats) {
        for (auto &player: players) {
            if (player->body && player->alive) player->body->draw(shader, frustum, stats);
        }
    }

    bool Replication::isRemoteBody(const GameObject *object) const {
        return object && std::any_of(players.begin(), players.end(),
                                     [object](const std::unique_ptr<RemotePlayer> &p) { return p->body.get() == object; });
    }

    void Replication::drawNameplates(const glm::mat4 &viewProjection) const {
        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImDrawList *draw = ImGui::GetBackgroundDrawList();
        for (const auto &player: players) {
            if (!player->hasCurrent) continue;
            const glm::vec4 clip = viewProjection * glm::vec4(nameplateAnchor(player->current.position), 1.f);
            if (clip.w <= 0.01f) continue; // behind the camera
            const glm::vec3 ndc = glm::vec3(clip) / clip.w;
            if (std::abs(ndc.x) > 1.1f || std::abs(ndc.y) > 1.1f) continue;
            const ImVec2 screen(viewport->WorkPos.x + (ndc.x * 0.5f + 0.5f) * viewport->WorkSize.x,
                                viewport->WorkPos.y + (0.5f - ndc.y * 0.5f) * viewport->WorkSize.y);

            const std::string label = player->alive ? player->name : player->name + " (dead)";
            const ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
            const ImVec2 textPos(screen.x - textSize.x * 0.5f, screen.y - textSize.y - 8.f);
            draw->AddText(ImVec2(textPos.x + 1.f, textPos.y + 1.f), IM_COL32(0, 0, 0, 200), label.c_str());
            draw->AddText(textPos, IM_COL32(255, 255, 255, 230), label.c_str());

            // Health bar: green to red, on a dark strip.
            constexpr float WIDTH = 64.f, HEIGHT = 6.f;
            const ImVec2 barMin(screen.x - WIDTH * 0.5f, screen.y - 6.f), barMax(screen.x + WIDTH * 0.5f, screen.y - 6.f + HEIGHT);
            const float fraction = std::clamp(static_cast<float>(player->health) / MAX_HEALTH, 0.f, 1.f);
            draw->AddRectFilled(barMin, barMax, IM_COL32(0, 0, 0, 160), 2.f);
            draw->AddRectFilled(barMin, ImVec2(barMin.x + WIDTH * fraction, barMax.y),
                                IM_COL32(static_cast<int>(255 * (1.f - fraction)), static_cast<int>(220 * fraction), 40, 230), 2.f);
        }
    }

    int Replication::getExtrapolatingCount() const {
        return static_cast<int>(std::count_if(players.begin(), players.end(),
                                              [](const std::unique_ptr<RemotePlayer> &p) { return p->extrapolating; }));
    }
}
