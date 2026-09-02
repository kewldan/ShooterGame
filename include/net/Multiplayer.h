#pragma once

#include "Audio.h"
#include "Effects.h"
#include "Player.h"
#include "net/NetClient.h"
#include "net/NetServer.h"
#include "net/Replication.h"
#include <memory>
#include <string>
#include <vector>

// Everything the game loop needs from the network in one place: with --host a NetServer in this
// process plus a client of it over the loopback interface (the host is a player like any other),
// with --connect just the client; without either it is inert and the game is single-player.
// It streams the local Player, keeps the remote ones (Replication), turns the server's events into
// effects, sounds, console lines and the local health, and draws the multiplayer parts of the HUD.
struct MultiplayerConfig {
    bool host = false;
    uint16_t port = 23403;            // --host [port]
    std::string connect;              // --connect ip[:port]; empty: not a client
    std::string name = "Player";      // --name
    float simulateLoss = 0.f;         // --simulate-loss P: drop this fraction of received packets
    int simulateLatencyMs = 0;        // --simulate-latency MS: hold received packets back this long
};

class Multiplayer {
    MultiplayerConfig config;
    btDynamicsWorld *world;
    Player &player;
    std::vector<std::unique_ptr<GameObject>> &crates;
    std::unique_ptr<net::NetServer> server;
    std::unique_ptr<net::NetClient> client;
    std::unique_ptr<net::Replication> replication;
    std::vector<net::ClientEvent> events; // scratch, reused every frame

    uint8_t localId = net::NO_PLAYER;
    int localHealth = net::MAX_HEALTH;
    uint16_t kills = 0, deaths = 0;
    float damageFlash = 0.f; // 1 right after being hit, fades out
    float deathFade = 0.f;   // 0 alive .. 1 dead (smoothed)
    double lastTime = 0.;    // the clock as of the last update(), for the goodbye in the destructor

    void handleEvent(const net::ClientEvent &event, Effects &effects, Audio &audio);

    // The server's world raycast: the host's Bullet world minus every player body.
    net::NetServer::WorldHit worldRaycast(const glm::vec3 &origin, const glm::vec3 &direction, float maxDistance,
                                          bool applyImpulse) const;

    [[nodiscard]] std::string nameOf(uint8_t id) const;

    void printPlayers() const;

public:
    // Server-side spawn points (eye positions) the dead come back at.
    static const std::vector<glm::vec3> SPAWN_POINTS;

    // `player` and `crates` must outlive the object; `now` is the game clock (glfwGetTime).
    Multiplayer(const MultiplayerConfig &config, btDynamicsWorld *world, Player &player,
                std::vector<std::unique_ptr<GameObject>> &crates, double now);

    ~Multiplayer();

    Multiplayer(const Multiplayer &) = delete;

    Multiplayer &operator=(const Multiplayer &) = delete;

    [[nodiscard]] bool isActive() const { return client != nullptr; }

    [[nodiscard]] bool isHost() const { return server != nullptr; }

    // Once per frame after Player::update(): sends the state and this frame's shot, receives, moves the
    // remote players and crates, and turns remote shots and hits into tracers, decals and sounds.
    void update(double now, float delta, const PlayerEvents &playerEvents, Effects &effects, Audio &audio);

    // The remote players' bodies, for every scene pass.
    void drawScene(Engine::Shader *shader, const Frustum *frustum, CullStats *stats);

    // Nameplates, the damage flash and the death fade (call between HUD::begin and HUD::end).
    void drawOverlay(const glm::mat4 &projection, const glm::mat4 &view);

    // Ping, packet rates and the player list, for the Debug tree.
    void drawDebugUi();

    // False for a local hit on a remote player's body (no bullet holes on people).
    [[nodiscard]] bool shouldDecal(const ShotResult &shot) const;

    [[nodiscard]] int getLocalHealth() const { return localHealth; }

    [[nodiscard]] int getKills() const { return kills; }

    [[nodiscard]] int getDeaths() const { return deaths; }

    [[nodiscard]] float getDeathFade() const { return deathFade; }

    [[nodiscard]] const std::string &getName() const { return config.name; }
};
