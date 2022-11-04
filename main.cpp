#include <reactphysics3d/reactphysics3d.h>

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
#include "MyProfiler.h"

int main() {
	std::remove("latest.log");
	plog::init(plog::debug, "latest.log");
	PLOG_INFO << "Logger initialized";

	PhysicsCommon physicsCommon;
	PhysicsWorld* world = physicsCommon.createPhysicsWorld();

	auto* profiler = new MyProfiler();

	profiler->startBlock("Init");
	auto* window = new Window();
	auto* shader = new Shader("main");
	auto lightPos = glm::vec3(-5.0f, 10.0f, -3.0f);
	auto lightLook = glm::vec3(0);
	auto* shadows = new ShadowsCaster(1024, 1024, "depth", 2.f, 30.f, &lightPos, &lightLook);

	auto* map = new Model("./data/meshes/map.obj", world, &physicsCommon, true);
	auto* player = new Model("./data/meshes/player.obj", world, &physicsCommon);
	auto* sniperRifle = new Model("./data/meshes/sniper.obj", world, &physicsCommon);
	sniperRifle->rb->addCollider(physicsCommon.createBoxShape(Vector3(0.06f, 0.26f, 1.35f)), Transform(Vector3(0, -0.02f, 0.19f), Quaternion::identity()));
	sniperRifle->rb->setMass(1);

	auto* sniperTexture = new Texture("data/textures/sniper.png");
	auto* mapTexture = new Texture("data/textures/palette.png");

	auto* camera = new Camera(window->getWidthPtr(), window->getHeightPtr(), window->getRatioPtr());
	camera->freeCamera = true;

	HUD* hud = new HUD(window);
	profiler->endBlock();

	while (window->update()) {
		static bool debugWindow, vsync = true, sort_invert = false, freeze = true, menuWindow = true;

		profiler->startBlock("Physics and update");
		camera->pollEvents(window);

		if (window->isKeyPressed(GLFW_KEY_E)) {
			//Add force of drop
			Vector3 position(camera->position.x, camera->position.y, camera->position.z);
			Transform newTransform(position, Quaternion::identity());
			sniperRifle->rb->setTransform(newTransform);
			sniperRifle->rb->setLinearVelocity(Vector3(0, 0, 0));
			sniperRifle->rb->setAngularVelocity(Vector3(0, 0, 0));
			sniperRifle->rb->applyLocalForceAtCenterOfMass(
				Vector3(
					std::cos(camera->rotation.y + 1.57f) * 100,
					30,
					std::sin(camera->rotation.y + 1.57f) * 100)
			);
		}

		if (!freeze) {
			world->update(ImGui::GetIO().DeltaTime);
		}
		profiler->endBlock();

		profiler->startBlock("Shadows");
		{
			Shader* depth = shadows->begin();
			depth->draw(sniperRifle);
			depth->draw(map);
			//depth->draw(player);
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
			shader->unbind();
		}
		profiler->endBlock();

		profiler->startBlock("HUD");
		{
			hud->begin();
			static const char* items[] = { "Average", "All time", "Iterations" };
			static int item_current = 0;

			ImGui::SetNextWindowPos(ImVec2(10, 170), ImGuiCond_Once);

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
					for (const auto& kv : profiler->blocks) {
						if (kv.second.iterations == 1) {
							once.emplace_back(kv);
						}
						else {
							others.emplace_back(kv);
						}
					}

					if (ImGui::TreeNode("Memory")) {
						MemoryInfo* info = profiler->getMemoryInfo();

						ImGui::Text("Virual: %.1f mb / %.1f mb (%.1f%%)", info->virtualMemoryUsed / 1024.f / 1024.f, info->virtualMemoryTotal / 1024.f / 1024.f, 100.f / (info->virtualMemoryTotal / (float)info->virtualMemoryUsed));
						ImGui::Text("Physical: %.1f mb / %.1f mb (%.1f%%)", info->physicalMemoryUsed / 1024.f / 1024.f, info->physicalMemoryTotal / 1024.f / 1024.f, 100.f / (info->physicalMemoryTotal / (float)info->physicalMemoryUsed));

						ImGui::TreePop();
						ImGui::Separator();
					}

					if (ImGui::TreeNode("Called once")) {
						for (const auto& kv : once) {
							ImGui::Text("%s: %d ms", kv.first.c_str(), kv.second.allTime);
						}
						ImGui::TreePop();
						ImGui::Separator();
					}


					std::sort(others.begin(), others.end(), [](std::pair<std::string, ProfilerBlock> const& lhs,
						std::pair<std::string, ProfilerBlock> const& rhs) -> bool {
							if (item_current == 0) {
								return ((float)lhs.second.allTime / (float)lhs.second.iterations) >
									((float)rhs.second.allTime / (float)rhs.second.iterations);
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
						if (ImGui::Button("Reset")) {
							for (const auto& kv : others) {
								profiler->blocks.erase(kv.first);
							}
						}
						ImGui::SameLine();
						ImGui::Combo("Sort by", &item_current, items, 3);
						ImGui::SameLine();
						ImGui::Checkbox("Inv", &sort_invert);
						ImGui::Spacing();
						for (const auto& kv : others) {
							if (ImGui::TreeNode(kv.first.c_str())) {
								ImGui::Text("Average: %.2f ms",
									(float)kv.second.allTime / (float)kv.second.iterations);
								ImGui::Text("CPU time: %d ms", kv.second.allTime);
								ImGui::Text("Iterations: %d", kv.second.iterations);
								ImGui::TreePop();
								ImGui::Separator();
							}
						}
						ImGui::TreePop();
					}
				}
			}
			ImGui::End();

			ImGui::SetNextWindowPos(ImVec2(300, 10), ImGuiCond_Once);

			if (ImGui::Begin("Menu", &menuWindow, ImGuiWindowFlags_AlwaysAutoResize)) {
				static char* nickname = new char[64];
				static char* ip = new char[16];
				nickname = (char*)"Player";
				ip = (char*)"255.255.255.255";

				ImGui::InputText("Nickname", nickname, 64, ImGuiInputTextFlags_NoHorizontalScroll);
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::InputText("IP", ip, 16, ImGuiInputTextFlags_NoHorizontalScroll);
				if (ImGui::Button("Connect")) {

				}
				ImGui::Separator();
				ImGui::Spacing();

				if (ImGui::Button("Host")) {

				}
			}
			ImGui::End();

			{
				ImGuiIO& io = ImGui::GetIO();
				ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
				ImGui::SetNextWindowBgAlpha(0.35f);
				if (ImGui::Begin("Debug overlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
					ImGuiWindowFlags_NoSavedSettings |
					ImGuiWindowFlags_NoFocusOnAppearing |
					ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove)) {
					ImGui::Text("Physics: %s", freeze ? "off" : "on");
					ImGui::Text("Screen: %dx%d", window->getWidth(), window->getHeight());
					ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
					ImGui::Text("Position: X: %.2f, Y: %.2f, Z: %.2f", camera->position.x, camera->position.y,
						camera->position.z);
					ImGui::Text("Rotation: X: %.2f, Y: %.2f", camera->rotation.x, camera->rotation.y);
					ImGui::Separator();
					ImGui::Spacing();
					ImGui::Text("Network: %s", "nothing");
				}
				ImGui::End();

				const float PAD = 20.0f;
				const ImGuiViewport* viewport = ImGui::GetMainViewport();
				ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
				ImVec2 work_size = viewport->WorkSize;
				ImVec2 window_pos, window_pos_pivot;
				window_pos.x = work_pos.x + work_size.x - PAD;
				window_pos.y = work_pos.y + work_size.y - PAD;
				window_pos_pivot.x = 1;
				window_pos_pivot.y = 1;
				ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
				ImGui::SetNextWindowBgAlpha(0.35f);

				if (ImGui::Begin("Info overlay", nullptr,
					ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
					ImGuiWindowFlags_NoSavedSettings |
					ImGuiWindowFlags_NoFocusOnAppearing |
					ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove)) {
					ImGui::Text("Health: %d / %d", 80, 100);
					ImGui::Text("Have weapon:");
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "no");
					ImGui::Text("Name: %s", "AK-47");
					ImGui::Text("Ammo: %d / %d", 17, 30);
				}
				ImGui::End();
			}


			window->setVsync(vsync);

			hud->end();
		}
		profiler->endBlock();
	}

	hud->destroy();
	window->destroy();
	shader->destroy();
	exit(EXIT_SUCCESS);
}