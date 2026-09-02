#include "Window.h"

#include "Camera3D.h"
#include "Texture.h"
#include "ShadowsCaster.h"
#include "Chat.h"

#include "imgui.h"
#include "HUD.h"
#include "imcmd_command_palette.h"

#include <Input.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include "Audio.h"
#include "Effects.h"
#include "GBuffer.h"
#include "Lights.h"
#include "Minimap.h"
#include "Player.h"
#include "PostProcess.h"
#include "Skybox.h"
#include "SSAO.h"
#include "Sun.h"
#include "Weapon.h"
#include "World.h"
#include "net/Multiplayer.h"

namespace {
    // Horizontal field of view, degrees (Engine::Camera3D converts it to the vertical one).
    constexpr float FOV_NORMAL = 90.f, FOV_AIM = 45.f;
    // The static map is split into cells of this size (world units, XZ) for frustum culling; dust.obj
    // (~224x246 units, 9.8k triangles, 27 materials) gives ~380 chunks with 32.
    constexpr float MAP_CHUNK_SIZE = 32.f;
    constexpr int SHADOW_MAP_SIZE = 2048; // per cascade
    // Shootable crates in front of the spawn point.
    constexpr int TARGET_COUNT = 3;
    constexpr float TARGET_HALF_SIZE = 0.5f, TARGET_MASS = 5.f;
    // Scripted test shots (--test-shots): interval and the fan they are spread over.
    constexpr double TEST_SHOT_INTERVAL = 0.35, TEST_SHOT_START = 1.0;
    constexpr float TEST_SHOT_SPREAD = 4.f; // degrees between two shots
    constexpr double SAY_START = 2.0;       // --say: seconds after start-up (the connection is up by then)

    // Optional start state from the command line (see parseArgs): handy for reproducible screenshots.
    struct Options {
        glm::vec3 position{0.f};
        float yaw = 0.f, pitch = 0.f; // radians
        bool vsync = true;
        int testShots = 0;            // fire this many shots in a fixed fan after start-up
        bool aim = false;             // keep the sights up
        float tracerLife = -1.f;      // override Effects::tracerLife (seconds) when >= 0
        MultiplayerConfig net;        // --host / --connect / --name / --simulate-*
        std::string say;              // a chat line sent shortly after start-up (screenshots of the chat)
        bool console = false;         // start with the console expanded
    };

    // --pos x y z (camera position), --yaw deg, --pitch deg, --novsync,
    // --test-shots N, --aim, --tracer-life seconds (debugging aids),
    // --host [port], --connect ip[:port], --name NAME, --simulate-loss P, --simulate-latency MS (multiplayer),
    // --say TEXT, --console (chat debugging aids)
    Options parseArgs(int argc, char **argv) {
        Options options;
        for (int i = 1; i < argc; i++) {
            const bool hasThree = i + 3 < argc, hasOne = i + 1 < argc;
            // A value that does not start with "--" (so that "--host --name X" works without a port).
            const bool hasValue = hasOne && std::strncmp(argv[i + 1], "--", 2) != 0;
            if (std::strcmp(argv[i], "--host") == 0) {
                options.net.host = true;
                if (hasValue) {
                    const int port = std::atoi(argv[++i]);
                    if (port > 0 && port <= 65535) {
                        options.net.port = static_cast<uint16_t>(port);
                    } else {
                        PLOGW << "Bad port [" << argv[i] << "], using " << options.net.port;
                    }
                }
            } else if (std::strcmp(argv[i], "--connect") == 0 && hasValue) {
                options.net.connect = argv[++i];
            } else if (std::strcmp(argv[i], "--name") == 0 && hasValue) {
                options.net.name = argv[++i];
            } else if (std::strcmp(argv[i], "--simulate-loss") == 0 && hasOne) {
                options.net.simulateLoss = std::strtof(argv[++i], nullptr);
            } else if (std::strcmp(argv[i], "--simulate-latency") == 0 && hasOne) {
                options.net.simulateLatencyMs = std::atoi(argv[++i]);
            } else if (std::strcmp(argv[i], "--say") == 0 && hasValue) {
                options.say = argv[++i];
            } else if (std::strcmp(argv[i], "--console") == 0) {
                options.console = true;
            } else if (std::strcmp(argv[i], "--pos") == 0 && hasThree) {
                options.position = glm::vec3(std::strtof(argv[i + 1], nullptr), std::strtof(argv[i + 2], nullptr),
                                             std::strtof(argv[i + 3], nullptr));
                i += 3;
            } else if (std::strcmp(argv[i], "--yaw") == 0 && hasOne) {
                options.yaw = glm::radians(std::strtof(argv[++i], nullptr));
            } else if (std::strcmp(argv[i], "--pitch") == 0 && hasOne) {
                options.pitch = glm::radians(std::strtof(argv[++i], nullptr));
            } else if (std::strcmp(argv[i], "--novsync") == 0) {
                options.vsync = false;
            } else if (std::strcmp(argv[i], "--test-shots") == 0 && hasOne) {
                options.testShots = std::atoi(argv[++i]);
            } else if (std::strcmp(argv[i], "--aim") == 0) {
                options.aim = true;
            } else if (std::strcmp(argv[i], "--tracer-life") == 0 && hasOne) {
                options.tracerLife = std::strtof(argv[++i], nullptr);
            } else {
                PLOGW << "Unknown argument: " << argv[i];
            }
        }
        return options;
    }

    // The one directional light of the scene: the direction the sunlight travels in (world space) and its
    // colour. Shared by the shadow pass, the lighting pass, the minimap, the sun sprite and the view-model
    // so they match. The scene is lit in linear HDR units (see PostProcess): the sun is a few times
    // brighter than white, the sky light reaching shadowed surfaces well below it.
    const glm::vec3 SUN_DIRECTION = glm::normalize(glm::vec3(3.5f, -7.f, 1.5f));
    const glm::vec3 SUN_COLOR(1.f, 0.95f, 0.85f);
    constexpr float SUN_INTENSITY = 4.f;
    const glm::vec3 AMBIENT_COLOR(0.5f, 0.54f, 0.62f);
    constexpr float SKY_INTENSITY = 1.8f;

    void applySsaoLevel(SSAO &ssao, int level) {
        ssao.visible = level > 0;
        if (level == 1) {
            ssao.bias = 0.2f;
            ssao.radius = 0.7f;
        } else if (level == 2) {
            ssao.bias = 0.1f;
            ssao.radius = 2.f;
        } else if (level == 3) {
            ssao.bias = 0.02f;
            ssao.radius = 3.f;
        }
    }

    // Dynamic crates a few units ahead of the player; they drop onto whatever the map has there.
    std::vector<std::unique_ptr<GameObject>> spawnTargets(btDynamicsWorld *world, const Player &player) {
        std::vector<std::unique_ptr<GameObject>> targets;
        const glm::vec3 forward = glm::normalize(glm::vec3(player.getForward().x, 0.f, player.getForward().z));
        const glm::vec3 right = player.getRight();
        for (int i = 0; i < TARGET_COUNT; i++) {
            const float side = static_cast<float>(i - TARGET_COUNT / 2) * 2.5f;
            const glm::vec3 p = player.getEyePosition() + forward * (9.f + static_cast<float>(i % 2) * 2.f) +
                                right * side;
            const float h = TARGET_HALF_SIZE;
            auto crate = std::make_unique<GameObject>(world, nullptr, TARGET_MASS, new btBoxShape(btVector3(h, h, h)),
                                                      btVector3(p.x, p.y, p.z));
            crate->setBoxMesh(glm::vec3(h), "de_dust2_material_12.png");
            targets.push_back(std::move(crate));
        }
        return targets;
    }

    // Everything that owns GL objects lives inside this function so that it is destroyed
    // before Engine::Window::destroy() tears the context down.
    void run(const Options &options) {
        auto window = std::make_unique<Engine::Window>(1280, 720, "Shooter game");
        if (options.net.host || !options.net.connect.empty()) {
            // Tells the windows apart when two instances share one desktop.
            const std::string title = "Shooter game - " + options.net.name + (options.net.host ? " (host)" : "");
            window->setTitle(title.c_str());
        }
        window->setVsync(options.vsync);
        auto input = std::make_unique<Engine::Input>(window->getId());
        input->registerCallbacks();
        auto camera = std::make_unique<Engine::Camera3D>(window.get());
        camera->setFov(options.aim ? FOV_AIM : FOV_NORMAL);

        glEnable(GL_DEPTH_TEST);

        Engine::HUD::init(window.get());
        ImCmd::CreateContext();

        auto world = std::make_unique<World>();

        auto skyShader = std::make_unique<Engine::Shader>("sky");
        auto shadows = std::make_unique<ShadowsCaster>(SHADOW_MAP_SIZE, "depth", SUN_DIRECTION);

        // The map collides with its own triangles; the flat box below it catches whatever falls through.
        auto map = std::make_unique<GameObject>(world->dynamicsWorld.get(), "dust.obj", 0.f, nullptr,
                                                btVector3(0.f, 0.f, 0.f), MAP_CHUNK_SIZE);
        auto catchFloor = std::make_unique<GameObject>(world->dynamicsWorld.get(), nullptr, 0.f,
                                                  new btBoxShape(btVector3(150.f, 1.f, 150.f)),
                                                  btVector3(0.f, -20.f, 0.f));
        auto player = std::make_unique<Player>(world->dynamicsWorld.get(), options.position, options.yaw, options.pitch);
        player->forceAim = options.aim;
        auto targets = spawnTargets(world->dynamicsWorld.get(), *player);

        auto weapon = std::make_unique<Weapon>("g17.obj");
        auto effects = std::make_unique<Effects>();
        if (options.tracerLife >= 0.f) {
            effects->tracerLife = options.tracerLife;
        }
        auto audio = std::make_unique<Audio>();
        for (const char *clip: {"gunshot", "dryfire", "reload", "footstep", "jump", "land", "hit"}) {
            audio->load(clip);
        }

        auto skybox = std::make_unique<Skybox>("sky");

        Chat::init();
        Chat::i->startExpanded = options.console;
        // Inert without --host/--connect; it owns the remote players and drives the crates on a client.
        auto multiplayer = std::make_unique<Multiplayer>(options.net, world->dynamicsWorld.get(), *player, targets,
                                                         glfwGetTime());

        auto minimap = std::make_unique<Minimap>("map", 512, 512, &camera->position, 60);

        auto ssao = std::make_unique<SSAO>("ssao", "ssaoBlur", window->width, window->height);

        auto gBuffer = std::make_unique<GBuffer>("pass1", "pass2", window->width, window->height,
                                                 ssao->ssaoColorBufferBlur, shadows->getMap());
        auto post = std::make_unique<PostProcess>(window->width, window->height, gBuffer->rboDepth);
        auto sun = std::make_unique<Sun>(-SUN_DIRECTION, SUN_COLOR * SUN_INTENSITY, window->width, window->height);
        auto pointLights = std::make_unique<PointLights>(gBuffer->gPosition, gBuffer->gNormal, gBuffer->gAlbedo);
        pointLights->lights = PointLights::mapLights();

        bool show_debugMenu = true, lockMouse = false, show_command_palette = false;
        bool vsync = options.vsync;
        bool visualizeCascades = false;
        bool wasAiming = options.aim;
        int ssaoLevel = 3;
        int testShotsLeft = options.testShots;
        double nextTestShot = glfwGetTime() + TEST_SHOT_START;
        double sayAt = options.say.empty() ? -1. : glfwGetTime() + SAY_START;
        // Frustum culling counters of the last frame, per pass (a skipped far cascade keeps its last values).
        CullStats geometryStats, minimapStats, shadowStats[ShadowsCaster::CASCADES];
        applySsaoLevel(*ssao, ssaoLevel);

        ImCmd::AddCommand({"Toggle chat", [] { Chat::i->visible = !Chat::i->visible; }});
        ImCmd::AddCommand({"Toggle debug overlay", [&] { show_debugMenu = !show_debugMenu; }});
        ImCmd::AddCommand({"Reset player", [&] { player->reset(glm::vec3(0.f)); }});
        ImCmd::AddCommand({"SSAO level",
                           [] { ImCmd::Prompt({"None", "Low", "Medium", "High"}); },
                           [&](int selected) {
                               ssaoLevel = selected;
                               applySsaoLevel(*ssao, ssaoLevel);
                           }});

        // The world geometry of every pass: the map, the crates and the other players; the player's own
        // body only where it is seen from outside (shadows, minimap).
        const auto drawScene = [&](Engine::Shader *shader, const Frustum &frustum, CullStats &stats, bool withPlayer) {
            map->draw(shader, &frustum, &stats);
            for (auto &target: targets) {
                target->draw(shader, &frustum, &stats);
            }
            multiplayer->drawScene(shader, &frustum, &stats);
            if (withPlayer) {
                player->body->draw(shader, &frustum, &stats);
            }
        };

        double lastTime = glfwGetTime();

        do {
            input->update();

            const double now = glfwGetTime();
            const float delta = static_cast<float>(now - lastTime);
            lastTime = now;

            glm::vec2 mouseDelta(0.f);
            if (lockMouse) {
                const glm::vec2 center(window->width / 2, window->height / 2);
                mouseDelta = input->getCursorPosition() - center;
                input->setCursorPosition(center);
            }

            // 1. Simulate the player (look, walk, jump, trigger) and the scripted test shots.
            PlayerEvents events = player->update(*input, mouseDelta, lockMouse, delta);
            if (testShotsLeft > 0 && now >= nextTestShot) {
                const int index = options.testShots - testShotsLeft;
                const float spread = glm::radians(TEST_SHOT_SPREAD);
                const float yawOffset = static_cast<float>(index - options.testShots / 2) * spread;
                const float pitchOffset = static_cast<float>(index % 3 - 1) * spread * 0.5f;
                const glm::vec3 forward = player->getForward(), right = player->getRight();
                const glm::vec3 up = glm::cross(right, forward);
                const glm::vec3 direction = glm::normalize(forward + right * std::tan(yawOffset) + up * std::tan(pitchOffset));
                player->fire(direction, events);
                testShotsLeft--;
                nextTestShot = now + TEST_SHOT_INTERVAL;
            }
            if (sayAt >= 0. && now >= sayAt) {
                Chat::i->submit(options.say.c_str());
                sayAt = -1.;
            }
            // Send the state and the shot, receive, move the other players; remote shots and hits become
            // effects, sounds and console lines in here.
            multiplayer->update(now, delta, events, *effects, *audio);

            const bool aiming = player->state.aiming;
            if (aiming != wasAiming) {
                camera->setFov(aiming ? FOV_AIM : FOV_NORMAL);
                wasAiming = aiming;
            }
            camera->position = player->getEyePosition();
            camera->rotation = glm::vec2(player->state.pitch, player->state.yaw);
            camera->update();
            const float aspect = static_cast<float>(window->width) / static_cast<float>(std::max(window->height, 1));
            weapon->update(mouseDelta, player->getHorizontalSpeed(), player->state.grounded, aiming, delta);

            // 2. Turn the events into sounds and effects.
            if (events.shot) {
                weapon->kick();
                audio->play("gunshot", 0.8f, 0.06f);
                const ShotResult &shot = events.lastShot;
                const glm::vec3 muzzle = weapon->getMuzzleWorld(camera->getProjection(), glm::inverse(camera->getView()),
                                                                aspect);
                if (shot.hit) {
                    if (multiplayer->shouldDecal(shot)) {
                        effects->addDecal(shot.point, shot.normal);
                    }
                    effects->addTracer(muzzle, shot.point);
                    if (shot.dynamic) {
                        audio->play("hit", 0.6f, 0.1f);
                    }
                } else {
                    effects->addTracer(muzzle, shot.origin + shot.direction * Player::SHOT_RANGE);
                }
            }
            if (events.dryFire) audio->play("dryfire", 0.5f);
            if (events.reloadStarted) audio->play("reload", 0.7f);
            if (events.jumped) audio->play("jump", 0.4f, 0.05f);
            if (events.landed) audio->play("land", 0.5f, 0.05f);
            if (events.footstep) audio->play("footstep", 0.35f, 0.15f);
            effects->update(delta);

            if (input->isKeyJustPressed(GLFW_KEY_ESCAPE)) {
                lockMouse = !lockMouse;
                if (lockMouse) {
                    input->hideCursor();
                } else {
                    input->showCursor();
                }
            }

            if (input->isKeyPressed(GLFW_KEY_LEFT_CONTROL) && input->isKeyPressed(GLFW_KEY_LEFT_SHIFT) &&
                input->isKeyJustPressed(GLFW_KEY_P)) {
                show_command_palette = !show_command_palette;
                if (show_command_palette) {
                    ImCmd::SetNextCommandPaletteSearchBoxFocused();
                }
            }

            if (input->isKeyJustPressed(GLFW_KEY_BACKSPACE)) {
                player->reset(glm::vec3(0.f));
            }

            world->update(delta);

            // 3. Shadows pass: one depth layer per cascade, culled with the light's ortho frustum
            if (shadows->visible) {
                shadows->pass(camera.get(), [&](Engine::Shader *shader, const Frustum &frustum, int cascade) {
                    shadowStats[cascade].reset();
                    drawScene(shader, frustum, shadowStats[cascade], true);
                });
            }

            // 4. Minimap pass
            minimapStats.reset();
            minimap->pass(camera->rotation.y, [&](Engine::Shader *shader, const Frustum &frustum) {
                shader->upload("sunDir", -SUN_DIRECTION);
                drawScene(shader, frustum, minimapStats, true);
            });

            if (window->isResized()) {
                gBuffer->resize(window->width, window->height);
                ssao->resize(window->width, window->height);
                post->resize(window->width, window->height);
                sun->resize(window->width, window->height);
            }

            // 5. Geometry pass [GBuffer], culled with the camera frustum
            geometryStats.reset();
            gBuffer->geometryPass(camera.get(), [&](Engine::Shader *shader, const Frustum &frustum) {
                drawScene(shader, frustum, geometryStats, false);
            });

            // 6. SSAO pass [SSAO]
            ssao->renderSSAOTexture(gBuffer->gPosition, gBuffer->gNormal, camera.get());

            // 7. SSAO blur [SSAO]
            ssao->blurSSAOTexture();

            // Everything from here to the post-processing is drawn in linear HDR into the HDR target,
            // which shares the depth of the G-buffer, so the forward passes are tested against the scene.
            const glm::vec3 sunDirView = glm::normalize(glm::mat3(camera->getView()) * -SUN_DIRECTION);
            const glm::vec3 sunColor = SUN_COLOR * SUN_INTENSITY;
            post->beginHdr();

            // 8. Lighting pass [GBuffer]: sun, ambient, SSAO and shadows, fullscreen
            gBuffer->lightingPass([&](Engine::Shader *shader) {
                shader->upload("SSAO", ssao->visible ? 1 : 0);
                // The G-buffer is in view space, so is the sun direction handed to the shader.
                shader->upload("sunDir", sunDirView);
                shader->upload("sunColor", sunColor);
                shader->upload("ambientColor", AMBIENT_COLOR);
                shader->upload("CastShadows", shadows->visible ? 1 : 0);
                shader->upload("visualizeCascades", visualizeCascades ? 1 : 0);
                if (shadows->visible) {
                    // The G-buffer stores view-space positions, the light matrices expect world space.
                    const glm::mat4 inverseView = glm::inverse(camera->getView());
                    for (int i = 0; i < ShadowsCaster::CASCADES; i++) {
                        const ShadowsCaster::Cascade &cascade = shadows->getCascade(i);
                        shader->upload(Engine::Shader::getElementName("lightSpaceMats", i),
                                       cascade.lightSpaceMatrix * inverseView);
                        shader->upload(Engine::Shader::getElementName("cascadeSplits", i), cascade.splitFar);
                        shader->upload(Engine::Shader::getElementName("cascadeTexelSizes", i), cascade.texelSize);
                        shader->upload(Engine::Shader::getElementName("cascadeDepthRanges", i), cascade.depthRange);
                    }
                }
            });

            // 9. Point lights [PointLights]: additive light volumes over the sunlit scene
            pointLights->drawVolumes(camera.get(), Frustum(camera->getProjection() * camera->getView()),
                                     window->width, window->height);

            // 10. Skybox draw
            skybox->draw(skyShader.get(), camera.get(), SKY_INTENSITY);

            // 11. Sun sprite and god rays [Sun]
            sun->draw(camera.get(), post->getHdrFBO(), post->getHdrTexture(), gBuffer->gNormal);

            // 12. Forward geometry and effects over the lit scene, tested against its depth: the light
            //     bulbs, then the decals and tracers
            pointLights->drawBulbs(camera.get());
            effects->draw(camera->getProjection(), camera->getView(), camera->position);

            // 13. View-model: own projection on a cleared depth buffer, so it never intersects the walls
            glClear(GL_DEPTH_BUFFER_BIT);
            weapon->draw(aspect, sunDirView, sunColor, AMBIENT_COLOR);

            // 14. Post-processing [PostProcess]: bloom, tone mapping and FXAA into the default framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            window->reset();
            post->render();

            // 15. HUD
            {
                Engine::HUD::begin();
                ImGui::SetNextWindowPos(ImVec2(15, 200), ImGuiCond_Once);

                if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    if (ImGui::TreeNode("Debug")) {
                        ImGui::SliderFloat("Speed", &player->speed, 0.1f, 10.f, "%.1f");
                        ImGui::Text("Grounded: %s, velocity: %.1f / %.1f", player->state.grounded ? "yes" : "no",
                                    player->getHorizontalSpeed(), player->state.velocity.y);
                        ImGui::SeparatorText("Frustum culling: drawn / culled chunks (draw calls)");
                        ImGui::Text("Map chunks: %d", static_cast<int>(map->chunks.size()));
                        ImGui::Text("Geometry: %d / %d (%d)", geometryStats.drawn, geometryStats.culled,
                                    geometryStats.drawCalls);
                        ImGui::Text("Minimap: %d / %d (%d)", minimapStats.drawn, minimapStats.culled,
                                    minimapStats.drawCalls);
                        for (int i = 0; i < ShadowsCaster::CASCADES; i++) {
                            ImGui::Text("Shadow cascade %d (to %.0f m): %d / %d (%d)", i,
                                        shadows->getCascade(i).splitFar, shadowStats[i].drawn,
                                        shadowStats[i].culled, shadowStats[i].drawCalls);
                        }
                        ImGui::Text("Point lights: %d / %d", pointLights->drawn, pointLights->culled);
                        ImGui::Text("Sun on screen: %.2f", sun->getFade());
                        multiplayer->drawDebugUi();
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("General")) {
                        ImGui::SliderFloat("Sensitivity", &player->sensitivity, 0.1f, 4.f, "%.1f");
                        float volume = audio->getMasterVolume();
                        if (ImGui::SliderFloat("Volume", &volume, 0.f, 1.f, "%.2f")) {
                            audio->setMasterVolume(volume);
                        }
                        ImGui::Checkbox("Show minimap", &minimap->visible);
                        if (minimap->visible) {
                            ImGui::Image((ImTextureID) (intptr_t) minimap->map, ImVec2(512, 512), ImVec2(0, 1),
                                         ImVec2(1, 0));
                        }
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Graphics")) {
                        if (ImGui::Checkbox("VSync", &vsync)) {
                            window->setVsync(vsync);
                        }
                        if (ImGui::Combo("SSAO level", &ssaoLevel, "None\0Low\0Medium\0High\0")) {
                            applySsaoLevel(*ssao, ssaoLevel);
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();
                            ImGui::Text("May cause performance issues");
                            ImGui::EndTooltip();
                        }

                        ImGui::Checkbox("Cast shadows", &shadows->visible);
                        if (shadows->visible) {
                            ImGui::Checkbox("Visualize cascades", &visualizeCascades);
                            ImGui::SliderInt("Update far cascades every N frames", &shadows->farCascadeInterval, 1, 8);
                            ImGui::SliderFloat("Shadow distance", &shadows->shadowDistance, 50.f, 300.f, "%.0f m");
                        }

                        ImGui::SeparatorText("HDR");
                        ImGui::SliderFloat("Exposure", &post->exposure, 0.1f, 4.f, "%.2f");
                        ImGui::Combo("Tone mapping", &post->tonemapper, "ACES\0Uncharted 2\0None\0");
                        ImGui::Checkbox("Bloom", &post->bloom);
                        if (post->bloom) {
                            ImGui::SliderFloat("Bloom strength", &post->bloomStrength, 0.f, 0.5f, "%.3f");
                            ImGui::SliderFloat("Bloom threshold", &post->bloomThreshold, 0.f, 5.f, "%.2f");
                        }
                        ImGui::Checkbox("FXAA", &post->fxaa);

                        ImGui::SeparatorText("Sun");
                        ImGui::Checkbox("God rays", &sun->godRays);
                        if (sun->godRays) {
                            ImGui::SliderFloat("God rays strength", &sun->raysStrength, 0.f, 2.f, "%.2f");
                            ImGui::SliderFloat("God rays density", &sun->raysDensity, 0.1f, 1.f, "%.2f");
                        }

                        ImGui::SeparatorText("Point lights");
                        ImGui::Checkbox("Point lights", &pointLights->visible);
                        if (pointLights->visible) {
                            ImGui::SliderFloat("Point light intensity", &pointLights->intensityScale, 0.f, 4.f, "%.2f");
                        }
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Weapon")) {
                        ImGui::SliderFloat("View-model FOV", &weapon->fov, 30.f, 90.f, "%.0f");
                        ImGui::SliderFloat("Scale", &weapon->scale, 0.02f, 0.5f, "%.3f");
                        ImGui::SliderFloat3("Hip offset", &weapon->hipOffset.x, -1.f, 1.f, "%.3f");
                        ImGui::SliderFloat3("Aim offset", &weapon->aimOffset.x, -1.f, 1.f, "%.3f");
                        ImGui::SliderFloat3("Base rotation", &weapon->baseRotation.x, -180.f, 180.f, "%.0f");
                        ImGui::SliderFloat("Sway", &weapon->swayAmount, 0.f, 0.0002f, "%.5f");
                        ImGui::SliderFloat("Bob", &weapon->bobAmount, 0.f, 0.05f, "%.3f");
                        ImGui::SliderFloat("Recoil angle", &weapon->recoilAngle, 0.f, 20.f, "%.1f");
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Effects")) {
                        ImGui::SliderFloat("Decal size", &effects->decalSize, 0.01f, 0.5f, "%.3f");
                        ImGui::SliderFloat("Tracer life", &effects->tracerLife, 0.02f, 2.f, "%.2f s");
                        ImGui::SliderFloat("Tracer width", &effects->tracerWidth, 0.002f, 0.1f, "%.3f");
                        ImGui::Text("Decals: %d / %d, tracers: %d", effects->getDecalCount(), Effects::MAX_DECALS,
                                    effects->getTracerCount());
                        ImGui::TreePop();
                    }
                }
                ImGui::End();

                {
                    if (show_debugMenu) {
                        ImGui::SetNextWindowPos(ImVec2(15, 15), ImGuiCond_Once);
                        ImGui::SetNextWindowBgAlpha(0.35f);

                        if (ImGui::Begin("##Debug overlay", nullptr,
                                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                         ImGuiWindowFlags_NoFocusOnAppearing |
                                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove)) {
                            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
                            ImGui::Text("Position: X: %.1f, Y: %.1f, Z: %.1f", camera->position.x,
                                        camera->position.y,
                                        camera->position.z);
                            ImGui::Text("Facing: %.1f / %.1f", camera->rotation.x, camera->rotation.y);
                            if (const ShotResult look = player->probe(); look.hit) {
                                ImGui::Text("Looking at: X: %.1f, Y: %.1f, Z: %.1f (%.1f away)", look.point.x,
                                            look.point.y, look.point.z, glm::distance(look.origin, look.point));
                            }
                        }
                        ImGui::End();
                    }

                    const ImGuiViewport *viewport = ImGui::GetMainViewport();
                    ImGui::SetNextWindowPos({
                                                    viewport->WorkPos.x + viewport->WorkSize.x - 15.f,
                                                    viewport->WorkPos.y + viewport->WorkSize.y - 15.f
                                            }, ImGuiCond_Always, {1, 1});
                    ImGui::SetNextWindowBgAlpha(0.f);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

                    if (ImGui::Begin("##Ammo overlay", nullptr,
                                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                     ImGuiWindowFlags_NoFocusOnAppearing |
                                     ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoInputs)) {
                        ImGui::Text("Health: %d / %d", multiplayer->getLocalHealth(), net::MAX_HEALTH);
                        if (multiplayer->isActive()) {
                            ImGui::Text("Kills: %d, deaths: %d", multiplayer->getKills(), multiplayer->getDeaths());
                        }
                        ImGui::NewLine();
                        ImGui::Text("Name: %s", "Glock 17");
                        ImGui::Text("Ammo: %d / %d", player->state.ammo, player->state.reserve);
                        if (player->state.reloading) {
                            ImGui::Text("Reloading... %.1f s", player->state.reloadTimer);
                        }
                    }
                    ImGui::End();
                    ImGui::PopStyleVar();

                    // Nameplates of the other players, the damage flash and the death fade.
                    multiplayer->drawOverlay(camera->getProjection(), camera->getView());

                    // Crosshair: a dot that shrinks while aiming.
                    ImDrawList *draw = ImGui::GetForegroundDrawList();
                    const ImVec2 centre(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
                                        viewport->WorkPos.y + viewport->WorkSize.y * 0.5f);
                    draw->AddCircleFilled(centre, 2.5f - weapon->getAim(), IM_COL32(255, 255, 255, 200));

                    if (Chat::i->visible) {
                        Chat::i->Draw();
                    }

                    if (show_command_palette) {
                        ImCmd::SetNextWindowAffixedTop(ImGuiCond_Once);
                        ImCmd::CommandPaletteWindow("Command palette", &show_command_palette);
                    }
                }
                Engine::HUD::end();
            }
        } while (window->update());

        Chat::i.reset();
        ImCmd::DestroyContext();
        Engine::HUD::destroy();
    }
}

int main(int argc, char **argv) {
    Engine::Window::init();
    run(parseArgs(argc, argv));
    Engine::Window::destroy();
    return EXIT_SUCCESS;
}
