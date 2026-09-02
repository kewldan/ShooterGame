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
#include <memory>
#include "Skybox.h"
#include "Minimap.h"
#include "GBuffer.h"
#include "SSAO.h"
#include "World.h"

namespace {
    constexpr float TWO_PI = 6.2831853f;
    constexpr float HALF_PI = 1.5707963f;
    constexpr float EYE_HEIGHT = 1.5f;   // camera offset above the capsule centre
    constexpr float JUMP_SPEED = 5.f;    // m/s
    constexpr float FOV_NORMAL = 60.f, FOV_AIM = 25.f;

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

    void resetPlayer(GameObject &player) {
        btTransform transform;
        transform.setIdentity();
        player.rb->setWorldTransform(transform);
        player.motionState->setWorldTransform(transform);
        player.rb->setLinearVelocity(btVector3(0.f, 0.f, 0.f));
        player.rb->setAngularVelocity(btVector3(0.f, 0.f, 0.f));
        player.rb->clearForces();
    }

    // Everything that owns GL objects lives inside this function so that it is destroyed
    // before Engine::Window::destroy() tears the context down.
    void run() {
        auto window = std::make_unique<Engine::Window>(1280, 720, "Shooter game");
        window->setVsync(true);
        auto input = std::make_unique<Engine::Input>(window->getId());
        input->registerCallbacks();
        auto camera = std::make_unique<Engine::Camera3D>(window.get());
        camera->setFov(FOV_NORMAL);

        glEnable(GL_DEPTH_TEST);

        Engine::HUD::init(window.get());
        ImCmd::CreateContext();

        auto world = std::make_unique<World>();

        auto skyShader = std::make_unique<Engine::Shader>("sky");
        auto shadows = std::make_unique<ShadowsCaster>(4096, 4096, "depth", 25.f);

        auto map = std::make_unique<GameObject>(world->dynamicsWorld.get(), "dust.obj", 0.f,
                                                new btBoxShape(btVector3(100.f, 1.f, 100.f)),
                                                btVector3(0.f, -10.f, 0.f));
        auto sniperRifle = std::make_unique<GameObject>(world->dynamicsWorld.get(), "g17.obj", 1.5f,
                                                        new btBoxShape(btVector3(1.f, 1.f, 1.f)));
        auto player = std::make_unique<GameObject>(world->dynamicsWorld.get(), "player.obj", 60.f,
                                                   new btCapsuleShape(1.f, 2.f));
        player->rb->setAngularFactor(0.f);
        player->rb->setSleepingThresholds(0.f, 0.f);

        auto skybox = std::make_unique<Skybox>("sky");

        Chat::init();

        auto minimap = std::make_unique<Minimap>("map", 512, 512, &camera->position, 60);

        auto ssao = std::make_unique<SSAO>("ssao", "ssaoBlur", window->width, window->height);

        auto gBuffer = std::make_unique<GBuffer>("pass1", "pass2", window->width, window->height,
                                                 ssao->ssaoColorBufferBlur, shadows->getMap());

        std::vector<Light> lights;

        float speed = 5.f;
        float sensitivity = 1.f;
        bool show_debugMenu = true, lockMouse = false, show_command_palette = false;
        bool vsync = true;
        int ssaoLevel = 3;
        applySsaoLevel(*ssao, ssaoLevel);

        ImCmd::AddCommand({"Toggle chat", [] { Chat::i->visible = !Chat::i->visible; }});
        ImCmd::AddCommand({"Toggle debug overlay", [&] { show_debugMenu = !show_debugMenu; }});
        ImCmd::AddCommand({"Reset player", [&] { resetPlayer(*player); }});
        ImCmd::AddCommand({"SSAO level",
                           [] { ImCmd::Prompt({"None", "Low", "Medium", "High"}); },
                           [&](int selected) {
                               ssaoLevel = selected;
                               applySsaoLevel(*ssao, ssaoLevel);
                           }});

        double lastTime = glfwGetTime();

        do {
            input->update();
            camera->update();

            const double now = glfwGetTime();
            const float delta = static_cast<float>(now - lastTime);
            lastTime = now;

            if (lockMouse) {
                const glm::vec2 center(window->width / 2, window->height / 2);
                glm::vec2 p = center - input->getCursorPosition();
                input->setCursorPosition(center);

                camera->rotation.x -= p.y * 0.001f * sensitivity;
                camera->rotation.x = std::clamp(camera->rotation.x, -1.5f, 1.5f);
                camera->rotation.y -= p.x * 0.001f * sensitivity;

                //fmod faster version
                if (camera->rotation.y >= TWO_PI) {
                    camera->rotation.y -= TWO_PI;
                }
                if (camera->rotation.y <= -TWO_PI) {
                    camera->rotation.y += TWO_PI;
                }

                if (input->isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
                    camera->setFov(FOV_AIM);
                }
                if (input->isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_RIGHT)) {
                    camera->setFov(FOV_NORMAL);
                }
            }

            const float slowWalk = input->isKeyPressed(GLFW_KEY_LEFT_SHIFT) ? 0.8f : 1.5f;
            const float moveSpeed = 5.f * speed * slowWalk;

            glm::vec3 vel(0.f);
            if (input->isKeyPressed(GLFW_KEY_W)) {
                vel.x -= std::cos(camera->rotation.y + HALF_PI) * moveSpeed;
                vel.z -= std::sin(camera->rotation.y + HALF_PI) * moveSpeed;
            } else if (input->isKeyPressed(GLFW_KEY_S)) {
                vel.x += std::cos(camera->rotation.y + HALF_PI) * moveSpeed;
                vel.z += std::sin(camera->rotation.y + HALF_PI) * moveSpeed;
            }

            if (input->isKeyPressed(GLFW_KEY_A)) {
                vel.x -= std::cos(camera->rotation.y) * moveSpeed;
                vel.z -= std::sin(camera->rotation.y) * moveSpeed;
            } else if (input->isKeyPressed(GLFW_KEY_D)) {
                vel.x += std::cos(camera->rotation.y) * moveSpeed;
                vel.z += std::sin(camera->rotation.y) * moveSpeed;
            }
            btVector3 current = player->rb->getLinearVelocity(); //Get current velocity (To save Y Axis velocity)
            current.setX(vel.x);
            current.setZ(vel.z);
            player->rb->setLinearVelocity(current);

            if (input->isKeyJustPressed(GLFW_KEY_SPACE)) {
                // An impulse (N*s) gives an instant velocity change; a force applied for one step does not.
                player->rb->applyCentralImpulse(btVector3(0.f, player->rb->getMass() * JUMP_SPEED, 0.f));
            }

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
                resetPlayer(*player);
            }

            btVector3 pos = player->rb->getWorldTransform().getOrigin();
            camera->position.x = pos.x();
            camera->position.y = pos.y() + EYE_HEIGHT;
            camera->position.z = pos.z();

            world->update(delta);

            // 1. Shadows pass TODO: Cascade shadow maps
            shadows->pass(camera->position, [&](Engine::Shader *shader) {
                sniperRifle->draw(shader);
                map->draw(shader);
                player->draw(shader);
            });

            // 2. Minimap pass
            minimap->pass(camera->rotation.y, [&](Engine::Shader *shader) {
                sniperRifle->draw(shader);
                map->draw(shader);
                player->draw(shader);
            });

            if (window->isResized()) {
                gBuffer->resize(window->width, window->height);
                ssao->resize(window->width, window->height);
            }

            // 3. Geometry pass [GBuffer]
            gBuffer->geometryPass(camera.get(), [&](Engine::Shader *shader) {
                map->draw(shader);
                sniperRifle->draw(shader);
            });

            // 4. SSAO pass [SSAO]
            ssao->renderSSAOTexture(gBuffer->gPosition, gBuffer->gNormal, camera.get());

            // 5. SSAO blur [SSAO]
            ssao->blurSSAOTexture();

            window->reset();

            // 6. Lighting pass [GBuffer]
            gBuffer->lightingPass(lights, [&](Engine::Shader *shader) {
                shader->upload("SSAO", ssao->visible ? 1 : 0);
                shader->upload("CastShadows", shadows->visible ? 1 : 0);
                if (shadows->visible) {
                    // The G-buffer stores view-space positions, the light matrix expects world space.
                    shader->upload("lightSpaceMat", shadows->getLightSpaceMatrix() * glm::inverse(camera->getView()));
                }
            });

            // 7. Skybox draw
            skybox->draw(skyShader.get(), camera.get());

            {
                Engine::HUD::begin();
                ImGui::SetNextWindowPos(ImVec2(15, 200), ImGuiCond_Once);

                if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    if (ImGui::TreeNode("Debug")) {
                        ImGui::SliderFloat("Speed", &speed, 0.1f, 10.f, "%.1f");
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("General")) {
                        ImGui::SliderFloat("Sensitivity", &sensitivity, 0.1f, 4.f, "%.1f");
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
                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();
                            ImGui::Text("WIP");
                            ImGui::EndTooltip();
                        }
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
                        ImGui::Text("Health: %d / %d", 100, 100);
                        ImGui::NewLine();
                        ImGui::Text("Name: %s", "Glock 17");
                        ImGui::Text("Ammo: %d / %d", 17, 17);
                    }
                    ImGui::End();
                    ImGui::PopStyleVar();

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

int main() {
    Engine::Window::init();
    run();
    Engine::Window::destroy();
    return EXIT_SUCCESS;
}
