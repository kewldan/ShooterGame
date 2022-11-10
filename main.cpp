#include "Shader.h"
#include <cstdlib>
#include "Window.h"

#include <plog/Log.h>
#include "plog/Initializers/RollingFileInitializer.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Appenders/ColorConsoleAppender.h"
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
	plog::get()->addAppender(new plog::ColorConsoleAppender<plog::TxtFormatter>());
	PLOG_INFO << "Logger initialized";

	auto* profiler = new MyProfiler();

	auto* window = new Window();
	auto* shader = new Shader("main");
	auto* debugShader = new Shader("debug");
	auto lightPos = glm::vec3(-7.0f, 10.0f, -3.0f);
	auto lightLook = glm::vec3(0);
	auto* shadows = new ShadowsCaster(4096, 4096, "depth", 2.f, 30.f, &lightPos, &lightLook);

	auto* map = new Model("./data/meshes/map.obj");
	auto* player = new Model("./data/meshes/player.obj");

	auto* sniperRifle = new Model("./data/meshes/sniper.obj");

	PLOGI << "Loading models and physics successfully";

	auto* sniperTexture = new Texture("data/textures/sniper.png");
	auto* mapTexture = new Texture("data/textures/palette.png");
	PLOGI << "Loading textures successfully";

	auto* camera = new Camera(window->getWidthPtr(), window->getHeightPtr(), window->getRatioPtr());

	HUD* hud = new HUD(window);
	PLOGI << "Loading HUD (ImGui) successfully";

	while (window->update()) {
		static bool debugWindow, menuWindow = true, wireframe, showControl = true;

		profiler->startBlock("Update");
		{
			camera->pollEvents(window);
		}
		profiler->endBlock();

		profiler->startBlock("Shadows");
		{
			Shader* depth = shadows->begin();
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
			shader->upload("environment.sun_position", lightPos);
			shader->upload("hasTexture", 1);
			shader->upload("displayWireframe", wireframe ? 1 : 0);
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
			ImGui::SetNextWindowPos(ImVec2(20, 170), ImGuiCond_Once);

			if (ImGui::Begin("Debug", &debugWindow, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
				if (ImGui::CollapsingHeader("Configuration")) {
					ImGui::SliderFloat("Camera speed", &camera->speed, 0.01f, 10.f);
					ImGui::Separator();
					static bool vsync = true;
					if (ImGui::Checkbox("VSync", &vsync)) {
						window->setVsync(vsync);
					}
					ImGui::Checkbox("Show wireframe", &wireframe);
					ImGui::Separator();
				}
				if (ImGui::CollapsingHeader("Profiling")) {
					if (ImGui::TreeNode("Called many times")) {
						if (ImGui::Button("Reset")) {
							profiler->blocks.clear();
						}
						ImGui::Spacing();
						if (ImGui::BeginTable("Table", 4, ImGuiTableFlags_Borders))
						{
							ImGui::TableSetupColumn("Block");
							ImGui::TableSetupColumn("Average");
							ImGui::TableSetupColumn("Time");
							ImGui::TableSetupColumn("Iterations");
							ImGui::TableHeadersRow();
							std::vector <std::pair<std::string, ProfilerBlock>> pairs;
							for (const auto& kv : profiler->blocks)
							{
								if (kv.second.iterations > 0) {
									pairs.emplace_back(kv);
								}
							}
							std::sort(pairs.begin(), pairs.end(), [](std::pair<std::string, ProfilerBlock> const& lhs,
								std::pair<std::string, ProfilerBlock> const& rhs) -> bool {
									return ((float)lhs.second.allTime / (float)lhs.second.iterations) >
										((float)rhs.second.allTime / (float)rhs.second.iterations);
								});
							for (const auto& kv : pairs )
							{
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text(kv.first.c_str());
								ImGui::TableNextColumn();
								ImGui::Text("%.2f ms", (float)kv.second.allTime / (float)kv.second.iterations);
								ImGui::TableNextColumn();
								ImGui::Text("%lu ms", kv.second.allTime);
								ImGui::TableNextColumn();
								ImGui::Text("%d", kv.second.iterations);
							}
							ImGui::EndTable();
						}
						ImGui::TreePop();
					}
				}
			}
			ImGui::End();

			ImGui::SetNextWindowPos(ImVec2(300, 20), ImGuiCond_Once);

			if (ImGui::Begin("Menu", &menuWindow, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
				static char* nickname = new char[64];
				static char* ip = new char[16];

				std::fill(nickname, nickname + 64, '\0');
				std::fill(ip, ip + 64, '\0');

				ImGui::InputText("Nickname", nickname, 64, ImGuiInputTextFlags_NoHorizontalScroll);
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::InputText("IP", ip, 16, ImGuiInputTextFlags_NoHorizontalScroll);
				if (ImGui::Button("Connect")) {

				}
				ImGui::SameLine();
				ImGui::Text("No message");
				ImGui::Separator();
				ImGui::Spacing();

				if (ImGui::Button("Host")) {

				}
				ImGui::SameLine();
				ImGui::Text("Offline");
			}
			ImGui::End();

			{
				ImGuiIO& io = ImGui::GetIO();
				ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Once);
				ImGui::SetNextWindowBgAlpha(0.35f);
				if (ImGui::Begin("Debug overlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
					ImGuiWindowFlags_NoSavedSettings |
					ImGuiWindowFlags_NoFocusOnAppearing |
					ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove)) {
					ImGui::Text("Screen: %dx%d", window->getWidth(), window->getHeight());
					ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
					ImGui::Text("Position: X: %.2f, Y: %.2f, Z: %.2f", camera->position.x, camera->position.y,
						camera->position.z);
					ImGui::Text("Rotation: X: %.2f, Y: %.2f", camera->rotation.x, camera->rotation.y);
				}
				ImGui::End();

				const float PAD = 20.0f;
				const ImGuiViewport* viewport = ImGui::GetMainViewport();
				{
					ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
					ImVec2 work_size = viewport->WorkSize;
					ImVec2 window_pos, window_pos_pivot;
					window_pos.x = work_pos.x + work_size.x - PAD;
					window_pos.y = work_pos.y + work_size.y - PAD;
					window_pos_pivot.x = 1;
					window_pos_pivot.y = 1;
					ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
					ImGui::SetNextWindowBgAlpha(0.35f);

					if (ImGui::Begin("Ammo overlay", nullptr,
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

				ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
				ImVec2 work_size = viewport->WorkSize;
				ImVec2 window_pos, window_pos_pivot;
				window_pos.x = work_pos.x + PAD;
				window_pos.y = work_pos.y + work_size.y - PAD;
				window_pos_pivot.x = 0;
				window_pos_pivot.y = 1;
				ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
				ImGui::SetNextWindowBgAlpha(0.35f);

				if (ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
					ImGuiWindowFlags_NoSavedSettings |
					ImGuiWindowFlags_NoFocusOnAppearing |
					ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove)) {
					ImGui::Text("Controls | To toggle press: Tab");
					ImGui::Text("WASD - walking");
					ImGui::Text("Shift - crouch");
					ImGui::Text("C - zoom");
					ImGui::Text("Escape - unlock/lock cursor");
					ImGui::Text("R - reload");
					ImGui::Text("RMB - ADS");
				}
				ImGui::End();
			}
			hud->end();
		}
		profiler->endBlock();
	}

	delete hud;
	delete shader;
	delete debugShader;
	delete player;
	delete map;
	delete sniperRifle;
	delete sniperTexture;
	delete mapTexture;
	delete shadows;
	delete window;
	system("pause");
	exit(EXIT_SUCCESS);
}