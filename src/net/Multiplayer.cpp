#include "net/Multiplayer.h"

#include "Chat.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <plog/Log.h>

namespace {
    constexpr float DAMAGE_FLASH_TIME = 0.6f; // seconds until the red vignette is gone
    constexpr float DEATH_FADE_SPEED = 2.5f;  // 1/s
    constexpr float DEATH_FADE_MAX = 0.8f;    // the screen never goes fully black

    // Closest hit that skips the player bodies (the shooter's and the replicated ones).
    struct WorldRayCallback : btCollisionWorld::ClosestRayResultCallback {
        const GameObject *localBody;
        const net::Replication *replication;

        WorldRayCallback(const btVector3 &from, const btVector3 &to, const GameObject *localBody,
                         const net::Replication *replication)
                : ClosestRayResultCallback(from, to), localBody(localBody), replication(replication) {
        }

        bool needsCollision(btBroadphaseProxy *proxy) const override {
            const auto *object = static_cast<const btCollisionObject *>(proxy->m_clientObject);
            const auto *owner = static_cast<const GameObject *>(object->getUserPointer());
            if (owner && (owner == localBody || (replication && replication->isRemoteBody(owner)))) {
                return false;
            }
            return ClosestRayResultCallback::needsCollision(proxy);
        }
    };

    glm::vec3 toGlm(const btVector3 &v) {
        return {v.x(), v.y(), v.z()};
    }

    btVector3 toBullet(const glm::vec3 &v) {
        return {v.x, v.y, v.z};
    }

    // Loudness of a shot fired `distance` units away from the listener.
    float attenuate(float distance) {
        return std::clamp(0.8f * (1.f - distance / 80.f), 0.08f, 0.8f);
    }
}

const std::vector<glm::vec3> Multiplayer::SPAWN_POINTS = {
        {0.f, -7.9f, 0.f}, {6.f, -7.9f, -6.f}, {-6.f, -7.9f, -6.f}, {0.f, -7.9f, -14.f},
};

Multiplayer::Multiplayer(const MultiplayerConfig &config, btDynamicsWorld *world, Player &player,
                         std::vector<std::unique_ptr<GameObject>> &crates, double now)
        : config(config), world(world), player(player), crates(crates) {
    if (config.host) {
        server = std::make_unique<net::NetServer>(
                config.port, SPAWN_POINTS,
                [this](const glm::vec3 &origin, const glm::vec3 &direction, float maxDistance, bool applyImpulse) {
                    return worldRaycast(origin, direction, maxDistance, applyImpulse);
                },
                [](const std::string &line) { PLOGI << "Server: " << line; }, now);
        if (!server->isOpen()) {
            Chat::i->print("[error] Could not open UDP port %u for hosting", config.port);
            server.reset();
            return;
        }
    }
    const bool joining = server != nullptr || !config.connect.empty();
    if (!joining) {
        return;
    }
    const net::Address address = server ? net::Address{0x7F000001, server->getPort()}
                                        : net::Address::parse(config.connect, net::DEFAULT_PORT);
    if (!address.isValid()) {
        Chat::i->print("[error] Bad server address: %s", config.connect.c_str());
        server.reset();
        return;
    }
    client = std::make_unique<net::NetClient>(address, config.name, now);
    if (!client->isOpen()) {
        Chat::i->print("[error] Could not open a UDP socket");
        client.reset();
        server.reset();
        return;
    }
    client->setSimulation(config.simulateLoss, config.simulateLatencyMs / 1000.);
    if (server) {
        server->setHostPort(client->getLocalPort());
        Chat::i->print("[success] Hosting on UDP port %u", server->getPort());
    } else {
        Chat::i->print("Connecting to %s...", address.toString().c_str());
    }
    // On the host the crates are simulated (and sent); a client only follows them.
    replication = std::make_unique<net::Replication>(world, server ? nullptr : &crates);

    Chat::i->onMessage = [this](const std::string &text) {
        if (client && client->getStatus() == net::NetClient::Status::Connected) {
            client->sendChat(text); // shown when the server echoes it, in the same order for everybody
        } else {
            Chat::i->print("%s: %s", this->config.name.c_str(), text.c_str());
        }
    };
    Chat::i->onCommand = [this](const std::string &command) {
        if (command == "?players") {
            printPlayers();
            return true;
        }
        return false;
    };
}

Multiplayer::~Multiplayer() {
    if (Chat::i) {
        Chat::i->onMessage = nullptr;
        Chat::i->onCommand = nullptr;
    }
    if (client) {
        client->leave(lastTime);
    }
}

net::NetServer::WorldHit Multiplayer::worldRaycast(const glm::vec3 &origin, const glm::vec3 &direction,
                                                   float maxDistance, bool applyImpulse) const {
    net::NetServer::WorldHit hit;
    const btVector3 from = toBullet(origin), to = toBullet(origin + direction * maxDistance);
    WorldRayCallback callback(from, to, player.body.get(), replication.get());
    world->rayTest(from, to, callback);
    if (!callback.hasHit()) {
        return hit;
    }
    hit.hit = true;
    hit.point = toGlm(callback.m_hitPointWorld);
    hit.distance = glm::distance(origin, hit.point);
    hit.normal = glm::normalize(toGlm(callback.m_hitNormalWorld));
    if (glm::dot(hit.normal, direction) > 0.f) hit.normal = -hit.normal;
    if (applyImpulse) {
        if (btRigidBody *body = btRigidBody::upcast(const_cast<btCollisionObject *>(callback.m_collisionObject));
            body && !body->isStaticOrKinematicObject()) {
            body->activate(true);
            body->applyImpulse(toBullet(direction * Player::SHOT_IMPULSE),
                               callback.m_hitPointWorld - body->getCenterOfMassPosition());
        }
    }
    return hit;
}

std::string Multiplayer::nameOf(uint8_t id) const {
    if (id == localId) return config.name;
    if (replication) {
        if (const net::Replication::RemotePlayer *remote = replication->find(id)) return remote->name;
    }
    return id == net::NO_PLAYER ? "server" : "#" + std::to_string(id);
}

void Multiplayer::printPlayers() const {
    Chat *chat = Chat::i.get();
    if (!isActive()) {
        chat->print("Single player: %s", config.name.c_str());
        return;
    }
    const float rtt = client->getStats().rtt;
    chat->print("#%u %s (you) ping %.0f ms, %d hp, %d/%d", localId, config.name.c_str(), std::max(rtt, 0.f) * 1000.f,
                localHealth, kills, deaths);
    for (const auto &remote: replication->getPlayers()) {
        chat->print("#%u %s ping %u ms, %d hp, %u/%u%s", remote->id, remote->name.c_str(), remote->ping, remote->health,
                    remote->kills, remote->deaths, remote->alive ? "" : " (dead)");
    }
}

void Multiplayer::handleEvent(const net::ClientEvent &event, Effects &effects, Audio &audio) {
    Chat *chat = Chat::i.get();
    if (const auto *accepted = std::get_if<net::AcceptedEvent>(&event)) {
        localId = accepted->msg.id;
        for (const net::PlayerInfo &info: accepted->msg.players) {
            if (info.id != localId) replication->addPlayer(info);
        }
        chat->print("[success] Connected as #%u, %d other player(s)", localId,
                    static_cast<int>(accepted->msg.players.size()) - 1);
    } else if (const auto *rejected = std::get_if<net::RejectedEvent>(&event)) {
        chat->print("[error] Rejected: %s", rejected->reason.c_str());
    } else if (const auto *disconnected = std::get_if<net::DisconnectedEvent>(&event)) {
        chat->print("[error] Disconnected: %s", disconnected->reason.c_str());
        replication->clear();
    } else if (const auto *joined = std::get_if<net::PlayerJoinedEvent>(&event)) {
        replication->addPlayer(joined->info);
        chat->print("%s joined", joined->info.name.c_str());
    } else if (const auto *left = std::get_if<net::PlayerLeftEvent>(&event)) {
        chat->print("%s left", nameOf(left->id).c_str());
        replication->removePlayer(left->id);
    } else if (const auto *fx = std::get_if<net::ShotFxMsg>(&event)) {
        // Somebody else fired: a tracer from about where their muzzle is, and the hole they made.
        glm::vec3 muzzle = fx->origin + fx->direction * 0.5f;
        if (const net::Replication::RemotePlayer *shooter = replication->find(fx->shooter)) {
            const glm::vec3 forward = shooter->getForward(), right = shooter->getRight();
            muzzle = shooter->getEyePosition() + forward * 0.5f + right * 0.15f - glm::cross(right, forward) * 0.1f;
        }
        effects.addTracer(muzzle, fx->point);
        if (fx->kind == net::ShotFxMsg::World) {
            effects.addDecal(fx->point, fx->normal);
        }
        audio.play("gunshot", attenuate(glm::distance(player.getEyePosition(), fx->origin)), 0.06f);
    } else if (const auto *hit = std::get_if<net::HitMsg>(&event)) {
        if (hit->victim == localId) {
            localHealth = hit->healthLeft;
            damageFlash = 1.f;
            if (localHealth == 0 && player.alive) {
                player.alive = false;
                chat->print("You were killed by %s", nameOf(hit->shooter).c_str());
            }
        } else if (net::Replication::RemotePlayer *victim = replication->find(hit->victim)) {
            victim->health = hit->healthLeft;
            if (hit->healthLeft == 0) {
                victim->alive = false;
                chat->print("%s killed %s", nameOf(hit->shooter).c_str(), victim->name.c_str());
            }
        }
        if (hit->shooter == localId) {
            audio.play("hit", 0.6f, 0.1f); // the hit marker
        }
    } else if (const auto *respawn = std::get_if<net::RespawnMsg>(&event)) {
        if (respawn->id == localId) {
            player.reset(respawn->position);
            player.alive = true;
            localHealth = net::MAX_HEALTH;
        } else if (net::Replication::RemotePlayer *remote = replication->find(respawn->id)) {
            remote->alive = true;
            remote->health = net::MAX_HEALTH;
        }
    } else if (const auto *message = std::get_if<net::ChatMsg>(&event)) {
        chat->print("%s: %s", nameOf(message->sender).c_str(), message->text.c_str());
    } else if (const auto *snapshot = std::get_if<net::SnapshotEvent>(&event)) {
        for (const net::Snapshot::Entry &entry: snapshot->snapshot.players) {
            if (entry.id == localId) {
                kills = entry.kills;
                deaths = entry.deaths;
            }
        }
        replication->applySnapshot(snapshot->receiveTime, snapshot->snapshot, localId);
    }
}

void Multiplayer::update(double now, float delta, const PlayerEvents &playerEvents, Effects &effects, Audio &audio) {
    lastTime = now;
    damageFlash = std::max(damageFlash - delta / DAMAGE_FLASH_TIME, 0.f);
    const float fadeTarget = player.alive ? 0.f : 1.f;
    deathFade += std::clamp(fadeTarget - deathFade, -DEATH_FADE_SPEED * delta, DEATH_FADE_SPEED * delta);
    if (!client) {
        return;
    }
    if (server) {
        std::vector<net::CrateState> states;
        states.reserve(crates.size());
        for (const auto &crate: crates) {
            const btTransform &transform = crate->rb->getWorldTransform();
            const btQuaternion q = transform.getRotation();
            states.push_back({toGlm(transform.getOrigin()), glm::quat(q.w(), q.x(), q.y(), q.z())});
        }
        server->setCrates(states);
        server->update(now);
    }
    events.clear();
    client->update(now, events);
    for (const net::ClientEvent &event: events) {
        handleEvent(event, effects, audio);
    }
    if (playerEvents.shot) {
        client->sendShot(playerEvents.lastShot.origin, playerEvents.lastShot.direction);
    }
    client->sendState(now, player.state, player.alive);
    replication->update(now);
}

void Multiplayer::drawScene(Engine::Shader *shader, const Frustum *frustum, CullStats *stats) {
    if (replication) replication->draw(shader, frustum, stats);
}

void Multiplayer::drawOverlay(const glm::mat4 &projection, const glm::mat4 &view) {
    if (replication) {
        replication->drawNameplates(projection * view);
    }
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImDrawList *draw = ImGui::GetBackgroundDrawList();
    const ImVec2 min = viewport->WorkPos;
    const ImVec2 max(min.x + viewport->WorkSize.x, min.y + viewport->WorkSize.y);
    if (damageFlash > 0.f) {
        // A red vignette: four edge strips fading towards the centre.
        const float alpha = damageFlash * damageFlash;
        const ImU32 edge = IM_COL32(200, 0, 0, static_cast<int>(alpha * 170.f)), clear = IM_COL32(200, 0, 0, 0);
        const float dx = viewport->WorkSize.x * 0.25f, dy = viewport->WorkSize.y * 0.3f;
        draw->AddRectFilledMultiColor(min, ImVec2(min.x + dx, max.y), edge, clear, clear, edge);
        draw->AddRectFilledMultiColor(ImVec2(max.x - dx, min.y), max, clear, edge, edge, clear);
        draw->AddRectFilledMultiColor(min, ImVec2(max.x, min.y + dy), edge, edge, clear, clear);
        draw->AddRectFilledMultiColor(ImVec2(min.x, max.y - dy), max, clear, clear, edge, edge);
    }
    if (deathFade > 0.f) {
        draw->AddRectFilled(min, max, IM_COL32(0, 0, 0, static_cast<int>(deathFade * DEATH_FADE_MAX * 255.f)));
        const char *text = "You died, respawning...";
        const ImVec2 size = ImGui::CalcTextSize(text);
        draw->AddText(ImVec2((min.x + max.x - size.x) * 0.5f, (min.y + max.y) * 0.5f + 40.f),
                      IM_COL32(255, 255, 255, static_cast<int>(deathFade * 255.f)), text);
    }
}

void Multiplayer::drawDebugUi() {
    ImGui::SeparatorText("Network");
    if (!client) {
        ImGui::Text("Single player");
        return;
    }
    const char *status = "connecting";
    switch (client->getStatus()) {
        case net::NetClient::Status::Connected: status = "connected"; break;
        case net::NetClient::Status::Rejected: status = "rejected"; break;
        case net::NetClient::Status::Disconnected: status = "disconnected"; break;
        default: break;
    }
    ImGui::Text("%s, %s as #%u", server ? "Host" : "Client", status, localId);
    const net::Connection::Stats &stats = client->getStats();
    ImGui::Text("Ping: %.0f ms", std::max(stats.rtt, 0.f) * 1000.f);
    ImGui::Text("In: %d pkt/s (%.1f kB/s), out: %d pkt/s (%.1f kB/s)", stats.packetsInPerSecond,
                stats.bytesInPerSecond / 1024.f, stats.packetsOutPerSecond, stats.bytesOutPerSecond / 1024.f);
    ImGui::Text("Packets lost: %u / %u", stats.packetsLost, stats.packetsSent);
    if (config.simulateLoss > 0.f || config.simulateLatencyMs > 0) {
        ImGui::Text("Simulating %.0f%% loss, %d ms latency", config.simulateLoss * 100.f, config.simulateLatencyMs);
    }
    if (server) {
        ImGui::Text("Server: %d player(s) on port %u", server->getPlayerCount(), server->getPort());
    }
    ImGui::SliderFloat("Interpolation delay", &replication->interpolationDelay, 0.f, 0.5f, "%.2f s");
    ImGui::Text("Remote players: %d (%d extrapolating)", static_cast<int>(replication->getPlayers().size()),
                replication->getExtrapolatingCount());
    for (const auto &remote: replication->getPlayers()) {
        ImGui::BulletText("#%u %s: %u ms, %d hp, %u/%u%s", remote->id, remote->name.c_str(), remote->ping,
                          remote->health, remote->kills, remote->deaths, remote->alive ? "" : " (dead)");
    }
}

bool Multiplayer::shouldDecal(const ShotResult &shot) const {
    return shot.hit && !(replication && replication->isRemoteBody(shot.object));
}
