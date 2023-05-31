#define STB_IMAGE_IMPLEMENTATION
#define GLFW_INCLUDE_NONE

#include "Window.h"
#include <mutex>

#include "Camera3D.h"
#include "Texture.h"
#include "ShadowsCaster.h"
#include "Chat.h"

#include "imgui.h"
#include "HUD.h"

#include <regex>
#include <Input.h>
#include "Skybox.h"
#include "Minimap.h"
#include "GBuffer.h"
#include "SSAO.h"
#include "World.h"
#include "GameObject.h"


Engine::Window *window;
Engine::Input *input;
Engine::Camera3D *camera;

World *world;

GameObject *map, *player, *sniperRifle;

float speed = 5.f;
Minimap *minimap;
GBuffer *gBuffer;
std::vector<Light> *lights;
SSAO *ssao;

const glm::vec3 lightPos(-3.5f, 10.f, -1.5f);

Chat *Chat::i = nullptr;

bool vsync = true, console_open = false, castShadows, show_minimap = true, show_ssao = true, show_debugMenu = true, lockMouse;

int main() {
    Engine::Window::init();

    window = new Engine::Window(1280, 720, "Shooter game");
    window->setVsync(vsync);
    input = new Engine::Input(window->getId());
    input->registerCallbacks();
    camera = new Engine::Camera3D(window);
    camera->setFov(60.f);

    glEnable(GL_DEPTH_TEST);

    Engine::HUD::init(window);

    world = new World();

    auto *skyShader = new Engine::Shader("sky");
    auto *shadows = new ShadowsCaster(4096, 4096, "depth", lightPos, 25.f);

    map = new GameObject(world->dynamicsWorld, "dust.obj", 0.f, new btBoxShape(btVector3(100.f, 1.f, 100.f)), btVector3(0.f, -10.f, 0.f));
    sniperRifle = new GameObject(world->dynamicsWorld, "G17.obj", 1.5f, new btBoxShape(btVector3(1.f, 1.f, 1.f)));
    player = new GameObject(world->dynamicsWorld, "player.obj", 60.f, new btCapsuleShape(1.f, 2.f));
    player->rb->setAngularFactor(0.f);

    auto *skybox = new Skybox("sky");

    Chat::i = new Chat();

    minimap = new Minimap("map", 512, 512, &camera->position, 60);

    ssao = new SSAO("ssao", "ssaoBlur", window->width, window->height);

    gBuffer = new GBuffer("pass1", "pass2", window->width, window->height, ssao->ssaoColorBufferBlur,
                          shadows->getMap());

    lights = new std::vector<Light>();

    do {
        input->update();
        camera->update();

        static float sensitivity = 1.f;

        if (lockMouse) {
            glm::vec2 p = glm::vec2(window->width / 2, window->height / 2) - input->getCursorPosition();
            input->setCursorPosition(glm::vec2(window->width / 2, window->height / 2));

            camera->rotation.x -= (float) p.y * 0.001f * sensitivity;
            camera->rotation.x = std::max(-1.5f, std::min(camera->rotation.x, 1.5f));
            camera->rotation.y -= (float) p.x * 0.001f * sensitivity;

            //fmod faster version
            if (camera->rotation.y >= 6.283f) {
                camera->rotation.y -= 6.283f;
            }
            if (camera->rotation.y <= -6.283f) {
                camera->rotation.y += 6.283f;
            }

            if (input->isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
                camera->setFov(25.f);
            }
            if (input->isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_RIGHT)) {
                camera->setFov(60.f);
            }
        }

        float crouch = input->isKeyPressed(GLFW_KEY_LEFT_SHIFT) ? 0.8f : 1.5f;

        glm::vec3 vel(0.f);
        if (input->isKeyPressed(GLFW_KEY_W)) {
            vel.x -= std::cos(camera->rotation.y + 1.57f) * 5.f * speed * crouch;
            vel.z -= std::sin(camera->rotation.y + 1.57f) * 5.f * speed * crouch;
        } else if (input->isKeyPressed(GLFW_KEY_S)) {
            vel.x += std::cos(camera->rotation.y + 1.57f) * 5.f * speed * crouch;
            vel.z += std::sin(camera->rotation.y + 1.57f) * 5.f * speed * crouch;
        }

        if (input->isKeyPressed(GLFW_KEY_A)) {
            vel.x -= std::cos(camera->rotation.y) * 5.f * speed * crouch;
            vel.z -= std::sin(camera->rotation.y) * 5.f * speed * crouch;
        } else if (input->isKeyPressed(GLFW_KEY_D)) {
            vel.x += std::cos(camera->rotation.y) * 5.f * speed * crouch;
            vel.z += std::sin(camera->rotation.y) * 5.f * speed * crouch;
        }
        btVector3 current = player->rb->getLinearVelocity(); //Get current velocity (To save Y Axis velocity)
        current.setX(vel.x);
        current.setZ(vel.z);
        player->rb->setLinearVelocity(current);
        if(input->isKeyJustPressed(GLFW_KEY_SPACE)) {
            player->rb->applyCentralForce(btVector3(0.f, 100.f, 0.f));
        }

        if(input->isKeyJustPressed(GLFW_KEY_ESCAPE)){
            lockMouse ^= 1;
            if (lockMouse) {
                input->hideCursor();
            } else {
                input->showCursor();
            }
        }

        if(input->isKeyJustPressed(GLFW_KEY_BACKSPACE)){
            btTransform transform;
            transform.setIdentity();
            player->rb->setWorldTransform(transform);
            player->rb->setLinearVelocity(btVector3(0.f, 0.f, 0.f));
        }

        btVector3 pos = player->rb->getWorldTransform().getOrigin();
        camera->position.x = pos.x();
        camera->position.y = pos.y() + 1.5f;
        camera->position.z = pos.z();

        world->update(ImGui::GetIO().DeltaTime);

        // 1. Shadows pass TODO: Cascade shadow maps
        if (castShadows) {
            Engine::Shader *depth = shadows->begin(camera->position);
            sniperRifle->draw(depth);
            map->draw(depth);
            player->draw(depth);
            shadows->end();
        }

        // 2. Minimap pass
        if (show_minimap) {
            minimap->pass(camera->rotation.y, [](Engine::Shader *shader) {
                sniperRifle->draw(shader);
                map->draw(shader);
                player->draw(shader);
            });
        }

        if (window->isResized()) {
            gBuffer->resize(window->width, window->height);
            ssao->resize(window->width, window->height);
        }

        // 3. Geometry pass [GBuffer]
        gBuffer->geometryPass(camera, [](Engine::Shader *shader) {
            glActiveTexture(GL_TEXTURE0);
            shader->upload("aTexture", 0);

            map->draw(shader);
        });

        if (show_ssao) {
            // 4. SSAO pass [SSAO]
            ssao->renderSSAOTexture(gBuffer->gPosition, gBuffer->gNormal, camera);

            // 5. SSAO blur [SSAO]
            ssao->blurSSAOTexture();
        }

        window->reset();

        // 6. Lighting pass [GBuffer]
        gBuffer->lightingPass(lights, camera, [&shadows](Engine::Shader *shader) {
            shader->upload("SSAO", show_ssao);
            shader->upload("CastShadows", castShadows);
            if (castShadows) {
                shader->upload("lightPos", lightPos);
                shader->upload("lightSpaceMat", shadows->getLightSpaceMatrix());
            }
        });

        // 7. Skybox draw
        skybox->draw(skyShader, camera);

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
                    ImGui::Checkbox("Show minimap", &show_minimap);
                    if (show_minimap) {
                        ImGui::Image((void *) (intptr_t) minimap->map, ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
                    }
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Graphics")) {
                    if (ImGui::Checkbox("VSync", &vsync)) {
                        window->setVsync(vsync);
                    }
                    static int ssaoLevel = 3;
                    if (ImGui::Combo("SSAO level", &ssaoLevel, "None\0Low\0Medium\0High\0")) {
                        show_ssao = ssaoLevel > 0;
                        if (ssaoLevel == 1) {
                            ssao->bias = 0.2f;
                            ssao->radius = 0.7f;
                        } else if (ssaoLevel == 2) {
                            ssao->bias = 0.1f;
                            ssao->radius = 2.f;
                        } else if (ssaoLevel == 3) {
                            ssao->bias = 0.02f;
                            ssao->radius = 3.f;
                        }
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("May cause performance issues");
                        ImGui::EndTooltip();
                    }

                    ImGui::Checkbox("Cast shadows", &castShadows);
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("WIP");
                        ImGui::EndTooltip();
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(300, 15), ImGuiCond_Once);

            {
                if (show_debugMenu) {
                    ImGui::SetNextWindowPos(ImVec2(15, 15), ImGuiCond_Once);
                    ImGui::SetNextWindowBgAlpha(0.35f);

                    if (ImGui::Begin("##Debug overlay", nullptr,
                                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                     ImGuiWindowFlags_NoFocusOnAppearing |
                                     ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove)) {
                        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
                        ImGui::Text("Position: X: %.1f, Y: %.1f, Z: %.1f", camera->position.x, camera->position.y,
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

                if (console_open) {
                    Chat::i->Draw();
                }

                if (strlen(Chat::i->message) > 0) {
                    memset(Chat::i->message, 0, 256);
                }
            }
            Engine::HUD::end();
        }
    } while (window->update());
    exit(EXIT_SUCCESS);
}
