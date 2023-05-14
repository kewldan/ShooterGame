#define STB_IMAGE_IMPLEMENTATION
#define GLFW_INCLUDE_NONE

#include <reactphysics3d/reactphysics3d.h>

#define _WINSOCKAPI_
#include <Windows.h>
#include "Shader.h"
#include <cstdlib>
#include "Window.h"
#include <mutex>

#include <plog/Log.h>
#include "Model.h"
#include "Camera3D.h"
#include "Texture.h"
#include "ShadowsCaster.h"
#include "Chat.h"

#include "imgui.h"
#include "HUD.h"
#include "Client.h"

#include <regex>
#include <Input.h>
#include "Skybox.h"
#include "Minimap.h"
#include "GBuffer.h"
#include "SSAO.h"

void TextCentered(const char *text) {
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(text).x) * 0.5f);
    ImGui::Text("%s", text);
}

Engine::Window *window;
Engine::Input *input;
Engine::Camera3D *camera;

PhysicsCommon physicsCommon;
PhysicsWorld *world;
Engine::Shader *debugShader, *skyShader;
ShadowsCaster *shadows;
Model *map, *player, *sniperRifle;

Client *client;
char *nickname, *ip;
double lastUpdate;
float sensitivity = 1.f, speed = 5.f;
Skybox *skybox;
Minimap *minimap;
GBuffer *gBuffer;
std::vector<Light> *lights;
std::thread cpl_thread;
SSAO *ssao;

const glm::vec3 lightPos(-3.5f, 10.f, -1.5f);
const std::regex ip_regex(R"((^((25[0-5]|(2[0-4]|1\d|[1-9]|)\d)\.?\b){4}$))");

Chat *Chat::i = nullptr;

bool vsync = true, physicsDebugRender, console_open = false, castShadows, show_minimap = true, show_ssao = true, show_debugMenu = true, lockMouse;

struct Enemy {
    unsigned int id;
    float x, y, z, rx, ry;
};

void key_callback(GLFWwindow *w, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (lockMouse) {
            if (key == GLFW_KEY_BACKSPACE) {
                player->rb->setTransform(Transform::identity());
                player->rb->setLinearVelocity(Vector3::zero());
            }
        }

        if (key == GLFW_KEY_ESCAPE) {
            lockMouse ^= 1;
            if(lockMouse){
                input->hideCursor();
            }else{
                input->showCursor();
            }
        } else if (key == GLFW_KEY_GRAVE_ACCENT) {
            console_open ^= 1;
        } else if (key == GLFW_KEY_F3) {
            show_debugMenu ^= 1;
        }
    }
}

void cpl_func() {
    while (client->isConnected()) {
        BasicPacket *packet = client->recivePacket();
        if (packet != nullptr) {
            if (packet->type == ServerPacketTypes::UPDATE) {

            } else if (packet->type == ServerPacketTypes::HANDSHAKE) {
                memcpy(&client->my_id, packet->payload, 4);
            } else if (packet->type == ServerPacketTypes::MESSAGE) {
                char sender_length, message_length;

                memcpy(&sender_length, packet->payload, 1);
                char *sender = new char[sender_length + 1];
                memcpy(sender, packet->payload + 1, sender_length);
                sender[sender_length] = 0;

                memcpy(&message_length, packet->payload + 1 + sender_length, 1);
                char *message = new char[message_length + 1];
                memcpy(message, packet->payload + 2 + sender_length, message_length);
                message[message_length] = 0;

                Chat::i->AddLog("[chat] [%s] %s", sender, message);
            } else if (packet->type == ServerPacketTypes::KICK) {
                char *reason = new char[packet->length + 1];
                memcpy(reason, packet->payload, packet->length);
                reason[packet->length] = 0;

                Chat::i->AddLog("[error] You kicked from server. Reason: %s\n", reason);
            } else if (packet->type == ServerPacketTypes::PLAYER_INFO) {
                char *dst = new char[packet->length - 4 + 1];
                memcpy(dst, packet->payload + 4, packet->length);
                dst[packet->length - 4] = 0;

                int id = 0;
                memcpy(&id, packet->payload, 4);
            } else {
                PLOGW << "Unknown packet type: " << packet->type;
            }
        } else {
            break;
        }
    }
}

int main() {
    Engine::Window::init();

    window = new Engine::Window(1280, 720, "Shooter game");
    window->setVsync(vsync);
    input = new Engine::Input(window->getId());
    camera = new Engine::Camera3D(window);
    camera->setFov(60.f);

    glfwSetKeyCallback(window->getId(), key_callback);

    Engine::HUD::init(window);

    world = physicsCommon.createPhysicsWorld();

    DebugRenderer &debugRenderer = world->getDebugRenderer();
    debugRenderer.reset();
    debugRenderer.setIsDebugItemDisplayed(DebugRenderer::DebugItem::COLLISION_SHAPE, true);
    world->setIsDebugRenderingEnabled(false);

    debugShader = new Engine::Shader("debug");
    skyShader = new Engine::Shader("sky");
    shadows = new ShadowsCaster(4096, 4096, "depth", lightPos, 25.f);

    map = new Model("dust.obj", world, &physicsCommon, true);
    sniperRifle = new Model("G17.obj", world, &physicsCommon);
    sniperRifle->rb->setType(BodyType::STATIC);

    int nb = -1;
    MeshData *data = Model::loadMesh("player.obj", &nb);

    CapsuleShape *playerShape = physicsCommon.createCapsuleShape(1.f, 2.5f);

    player = new Model(data, nb, world, &physicsCommon);
    player->rb->setTransform(Transform(Vector3(0, 20, 0), Quaternion::identity()));
    player->rb->addCollider(playerShape, Transform(Vector3(0, 1.2f, 0), Quaternion::identity()));
    player->rb->setAngularLockAxisFactor(Vector3::zero());

    delete[] data;

    skybox = new Skybox("sky");

    Chat::i = new Chat();

    minimap = new Minimap("map", 512, 512, &camera->position, 60);

    ssao = new SSAO("ssao", "ssaoBlur", window->width, window->height);

    gBuffer = new GBuffer("pass1", "pass2", window->width, window->height, ssao->ssaoColorBufferBlur, shadows->getMap());

    lights = new std::vector<Light>();

    client = new Client();
    nickname = new char[32];
    DWORD username_len = 32;
    GetUserNameA(nickname, &username_len);

    ip = new char[16];
    strcpy_s(ip, 16, "127.0.0.1");
    do {
        input->update();
        camera->update();

        if (lockMouse) {
            glm::vec2 p = glm::vec2(window->width / 2, window->height / 2) - input->getCursorPosition();
            input->setCursorPosition(glm::vec2(window->width / 2, window->height / 2));

            camera->rotation.x -= (float)p.y * 0.001f * sensitivity;
            camera->rotation.x = std::max(-1.5f, std::min(camera->rotation.x, 1.5f));
            camera->rotation.y -= (float)p.x * 0.001f * sensitivity;

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
            if(input->isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_RIGHT)){
                camera->setFov(60.f);
            }

            if(input->isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)){
                Vector3 startPoint = player->rb->getTransform().getPosition();
                Vector3 endPoint(
                        startPoint.x + std::cos(camera->rotation.y - 1.57f) * 50.f,
                        startPoint.y + std::sin(camera->rotation.x) * 10.f,
                        startPoint.z + std::sin(camera->rotation.y - 1.57f) * 50.f
                );
                Ray ray(startPoint, endPoint);
                RaycastInfo raycastInfo;
            }
        }

        float crouch = input->isKeyPressed(GLFW_KEY_LEFT_SHIFT) ? 0.8f : 1.5f;

        Vector3 vel = Vector3::zero();
        if (input->isKeyPressed(GLFW_KEY_W)) {
            vel.x -= std::cos(camera->rotation.y + 1.57f) * 5.f * speed * crouch;
            vel.z -= std::sin(camera->rotation.y + 1.57f) * 5.f * speed * crouch;
        }
        else if (input->isKeyPressed(GLFW_KEY_S)) {
            vel.x += std::cos(camera->rotation.y + 1.57f) * 5.f * speed * crouch;
            vel.z += std::sin(camera->rotation.y + 1.57f) * 5.f * speed * crouch;
        }

        if (input->isKeyPressed(GLFW_KEY_A)) {
            vel.x -= std::cos(camera->rotation.y) * 5.f * speed * crouch;
            vel.z -= std::sin(camera->rotation.y) * 5.f * speed * crouch;
        }
        else if (input->isKeyPressed(GLFW_KEY_D)) {
            vel.x += std::cos(camera->rotation.y) * 5.f * speed * crouch;
            vel.z += std::sin(camera->rotation.y) * 5.f * speed * crouch;
        }
        Vector3 current = player->rb->getLinearVelocity(); //Get current velocity (To save Y Axis velocity)
        current.x = vel.x;
        current.z = vel.z;
        player->rb->setLinearVelocity(current);

        Vector3 pos = player->rb->getTransform().getPosition();
        camera->position.x = pos.x;
        camera->position.y = pos.y + 1.5f;
        camera->position.z = pos.z;

        if (client->isConnected()) {
            if (glfwGetTime() - lastUpdate > 0.02f) {
                Vector3 pos = player->rb->getTransform().getPosition();
                client->sendUpdate(pos.x, pos.y, pos.z, camera->rotation.x, camera->rotation.y);
                lastUpdate = glfwGetTime();
            }
        }
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
            minimap->pass(camera->rotation.y, [](Engine::Shader* shader){
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
        gBuffer->geometryPass(camera, [](Engine::Shader* shader){
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
        gBuffer->lightingPass(lights, camera, [](Engine::Shader* shader){
            shader->upload("SSAO", show_ssao);
            shader->upload("CastShadows", castShadows);
            if (castShadows) {
                shader->upload("lightPos", lightPos);
                shader->upload("lightSpaceMat", shadows->getLightSpaceMatrix());
            }
        });

        // 7. Skybox draw
        skybox->draw(skyShader, camera);

        if (world->getIsDebugRenderingEnabled()) {
            static unsigned int VAO = -1, VBO;

            if (VAO == -1) {
                glGenVertexArrays(1, &VAO);
                glBindVertexArray(VAO);

                glGenBuffers(1, &VBO);
                glBindBuffer(GL_ARRAY_BUFFER, VBO);

                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, false,
                                      4 * sizeof(float), nullptr);
            }

            {
                unsigned int verticesCount = debugRenderer.getNbTriangles() * 12U;
                if (verticesCount > 0) {
                    glBindVertexArray(VAO);
                    glBindBuffer(GL_ARRAY_BUFFER, VBO);
                    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(float), debugRenderer.getTrianglesArray(),
                                 GL_DYNAMIC_DRAW);

                    debugShader->bind();
                    debugShader->upload("proj", camera->getProjection());
                    debugShader->upload("view", camera->getView());
                    glDrawArrays(GL_TRIANGLES, 0, verticesCount);
                }
            }
        }

        {
            Engine::HUD::begin();
            ImGui::SetNextWindowPos(ImVec2(15, 200), ImGuiCond_Once);

            if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                if (client->isConnected()) {
                    ImGui::BeginDisabled();
                }
                ImGui::InputText("IP", ip, 16,
                                 ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_CharsScientific);
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                    ImGui::BeginTooltip();
                    ImGui::Text("IPv4 address\nFormat: XXX.XXX.XXX.XXX");
                    ImGui::EndTooltip();
                }
                ImGui::InputText("Nickname", nickname, 32, ImGuiInputTextFlags_NoHorizontalScroll);
                if (client->isConnected()) {
                    ImGui::EndDisabled();
                }

                bool canConnect = std::regex_match(ip, ip_regex) && strcmp(nickname, "Server") != 0;

                if (!canConnect && !client->isConnected()) {
                    ImGui::BeginDisabled();
                }

                if (ImGui::Button(client->isConnected() ? "Disconnect" : "Connect", ImVec2(270, 20))) {
                    if (!client->isConnected()) {
                        client->connectToHost(ip, NETWORKING_PORT);
                        if (client->isConnected()) {
                            cpl_thread = std::thread(cpl_func);
                            cpl_thread.detach();
                            client->sendHandshake(nickname);
                        }
                    } else {
                        client->disconnectFromHost();
                    }
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                    ImGui::BeginTooltip();
                    if (!std::regex_match(ip, ip_regex)) {
                        ImGui::Text("Incorrect IP address");
                    } else if (strcmp(nickname, "Server") == 0) {
                        ImGui::Text("Incorrect nickname");
                    } else {
                        ImGui::Text("Connect to %s:%d", ip, NETWORKING_PORT);
                    }
                    ImGui::EndTooltip();
                }

                if (!canConnect && !client->isConnected()) {
                    ImGui::EndDisabled();
                }

                if (ImGui::TreeNode("Debug")) {
                    ImGui::SliderFloat("Speed", &speed, 0.1f, 10.f, "%.1f");
                    if (ImGui::Checkbox("Debug render", &physicsDebugRender)) {
                        world->setIsDebugRenderingEnabled(physicsDebugRender);
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("Physics debug colliders render");
                        ImGui::EndTooltip();
                    }
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
                    if (ImGui::Combo("SSAO level", &ssaoLevel, "None\0Low\0Medium\0High")) {
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

                if (input->isKeyPressed(GLFW_KEY_TAB)) {
                    ImGui::SetNextWindowPos({
                                                    viewport->WorkSize.x / 2,
                                                    viewport->WorkSize.y / 2
                                            }, ImGuiCond_Always, {0.5f, 0.5f});
                    ImGui::SetNextWindowBgAlpha(0.35f);
                    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);

                    if (ImGui::Begin("##Server", nullptr,
                                     ImGuiWindowFlags_NoDecoration |
                                     ImGuiWindowFlags_NoFocusOnAppearing |
                                     ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoInputs)) {
                        if (client->isConnected()) {
                            TextCentered("Online (TCP/IP)");
                            ImGui::Text("ID: %d", client->my_id);

                        } else {
                            TextCentered("Offline");
                        }
                    }
                    ImGui::End();
                }

                if (console_open) {
                    Chat::i->Draw();
                }

                if (strlen(Chat::i->buffer) > 0) {
                    if (client->isConnected()) {
                        client->sendMessage(Chat::i->buffer);
                    }
                    memset(Chat::i->buffer, 0, 256);
                }
            }
            Engine::HUD::end();
        }
    } while (window->update());
    physicsCommon.destroyPhysicsWorld(world);
    exit(EXIT_SUCCESS);
}
