#include "Shader.h"
#include <cstdlib>
#include "Window.h"

#include <plog/Log.h>
#include "plog/Initializers/RollingFileInitializer.h"
#include "Model.h"
#include "Camera.h"
#include "Texture.h"
#include "ShadowsCaster.h"

#include "imgui.h"
#include "HUD.h"
#include "Profiler.h"

int main() {
    std::remove("latest.log");
    plog::init(plog::debug, "latest.log");
    PLOG_INFO << "Logger initialized";

    auto *profiler = new Profiler();

    profiler->startBlock("Init");
    auto *window = new Window();
    auto *shader = new Shader("main");
    auto lightPos = glm::vec3(-5.0f, 10.0f, -3.0f);
    auto lightLook = glm::vec3(0);
    auto *shadows = new ShadowsCaster(4096, 4096, "depth", 0.1f, 40.f, &lightPos, &lightLook);

    auto *sniperRifle = new Model("./data/meshes/sniper.obj");
    sniperRifle->scale *= 2.f;
    auto *map = new Model("./data/meshes/map.obj");
    map->scale *= 10.f;
    auto *player = new Model("./data/meshes/player.obj");

    auto *sniperTexture = new Texture("data/textures/sniper.png");
    auto *mapTexture = new Texture("data/textures/palette.png");

    auto *camera = new Camera(window->getWidthPtr(), window->getHeightPtr(), window->getRatioPtr());

    HUD *hud = new HUD(window);
    profiler->endBlock();

    while (window->update()) {
        static bool debugWindow, vsync = true, sort_invert = false, freeze = false;

        profiler->startBlock("Update");
        camera->pollEvents(window);
        if (window->isKeyPressed(GLFW_KEY_LEFT_CONTROL) && window->isKeyPressed(GLFW_KEY_F)) {
            freeze ^= 1;
        }

        if (window->isKeyPressed(GLFW_KEY_E)) {
            sniperRifle->position = camera->position;
            sniperRifle->velocity.x = std::cos(camera->rotation.y - 1.57f) * 3.f;
            sniperRifle->velocity.y = 4.f;
            sniperRifle->velocity.z = std::sin(camera->rotation.y - 1.57f) * 3.f;
        }

        if (!freeze) {
            sniperRifle->rotation.y = (float) glfwGetTime();
            sniperRifle->update(window->getTimeScale());
            player->position = camera->position;
            player->position.y -= 1.2f;
        }
        profiler->endBlock();

        profiler->startBlock("Shadows");
        {
            Shader *depth = shadows->begin();
            depth->draw(sniperRifle);
            depth->draw(map);
            depth->draw(player);
            shadows->end();
        }
        profiler->endBlock();

        profiler->startBlock("Render");
        {
            window->reset();
            shader->bind();
            shader->upload("proj", camera->getPerspective());
            shader->upload("camera.transform", camera->getView());
            shader->upload("camera.position", camera->position);
            shader->upload("environment.sun_position", lightPos);
            shader->upload("hasTexture", 1);
            shader->upload("aTexture", 1);
            shader->upload("shadowMap", 0);
            shader->upload("lightSpaceMatrix", *(shadows->getLightSpaceMatrix()));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, shadows->getMap());
            glActiveTexture(GL_TEXTURE1);
            sniperTexture->bind();
            shader->draw(sniperRifle);
            mapTexture->bind();
            shader->draw(map);
            shader->upload("hasTexture", 0);
            shader->draw(player);
            shader->unbind();
        }
        profiler->endBlock();

        profiler->startBlock("HUD");
        {
            hud->begin();
            static const char *items[] = {"Average", "All time", "Iterations"};
            static int item_current = 0;

            ImGui::SetNextWindowPos(ImVec2(10, 120), ImGuiCond_Once);

            if (ImGui::Begin("Debug", &debugWindow, ImGuiWindowFlags_AlwaysAutoResize)) {
                if (ImGui::CollapsingHeader("Configuration")) {
                    ImGui::SliderFloat("Camera speed", &camera->speed, 0.01f, 10.f);
                    ImGui::Separator();
                    ImGui::Checkbox("VSync", &vsync);
                    ImGui::Checkbox("Freeze", &freeze);
                    ImGui::Separator();
                }
                if (ImGui::CollapsingHeader("Profiling")) {
                    std::vector<std::pair<std::string, ProfilerBlock>> once;
                    std::vector<std::pair<std::string, ProfilerBlock>> others;
                    for (const auto &kv: profiler->blocks) {
                        if (kv.second.iterations == 1) {
                            once.emplace_back(kv);
                        } else {
                            others.emplace_back(kv);
                        }
                    }

                    if (ImGui::TreeNode("Called once")) {
                        for (const auto &kv: once) {
                            ImGui::Text("%s: %d ms", kv.first.c_str(), kv.second.allTime);
                        }
                        ImGui::TreePop();
                        ImGui::Separator();
                    }


                    std::sort(others.begin(), others.end(), [](std::pair<std::string, ProfilerBlock> const &lhs,
                                                               std::pair<std::string, ProfilerBlock> const &rhs) -> bool {
                        if (item_current == 0) {
                            return ((float) lhs.second.allTime / (float) lhs.second.iterations) >
                                   ((float) rhs.second.allTime / (float) rhs.second.iterations);
                        }
                        if (item_current == 1) {
                            return lhs.second.allTime > rhs.second.allTime;
                        }
                        if (item_current == 2) {
                            return lhs.second.iterations > rhs.second.iterations;
                        }
                        return false;
                    });

                    if (ImGui::TreeNode("Called many times")) {
                        if (ImGui::Button("Reset all")) {
                            for (const auto &kv: others) {
                                profiler->blocks.erase(kv.first);
                            }
                        }
                        ImGui::SameLine();
                        ImGui::Combo("Sort by", &item_current, items, 3);
                        ImGui::SameLine();
                        ImGui::Checkbox("Inv", &sort_invert);
                        ImGui::Spacing();
                        for (const auto &kv: others) {
                            if (ImGui::TreeNode(kv.first.c_str())) {
                                ImGui::Text("Average: %.2f ms",
                                            (float) kv.second.allTime / (float) kv.second.iterations);
                                ImGui::Text("CPU time: %d ms", kv.second.allTime);
                                ImGui::Text("Iterations: %d", kv.second.iterations);
                                ImGui::SameLine();
                                if (ImGui::Button("Reset")) {
                                    profiler->blocks.erase(kv.first);
                                }
                                ImGui::TreePop();
                                ImGui::Separator();
                            }
                        }
                        ImGui::TreePop();
                    }
                }
                ImGui::End();
            }

            ImGuiIO &io = ImGui::GetIO();
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
            ImGui::SetNextWindowBgAlpha(0.35f);
            if (ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                                 ImGuiWindowFlags_NoSavedSettings |
                                                 ImGuiWindowFlags_NoFocusOnAppearing |
                                                 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove)) {
                ImGui::Text("Ultra graphics new shooter game");
                ImGui::Separator();
                ImGui::Text("Physics: %s", freeze ? "off" : "on");
                ImGui::Text("Screen: %dx%d", window->getWidth(), window->getHeight());
                ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
                ImGui::Text("Position: X: %.2f, Y: %.2f, Z: %.2f", camera->position.x, camera->position.y,
                            camera->position.z);
                ImGui::Text("Rotation: X: %.2f, Y: %.2f", camera->rotation.x, camera->rotation.y);
            }
            ImGui::End();

            window->setVsync(vsync);
            camera->freeCamera = freeze;

            hud->end();
        }
        profiler->endBlock();
    }

    hud->destroy();
    window->destroy();
    shader->destroy();
    exit(EXIT_SUCCESS);
}