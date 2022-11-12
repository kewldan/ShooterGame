#include <reactphysics3d/reactphysics3d.h>

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

//#define RMT_PROFILER

#include "imgui.h"
#include "HUD.h"
#include "Client.h"
#ifdef RMT_PROFILER
#include "Remotery.h"
#endif
#include <regex>

#ifndef NDEBUG
void GLAPIENTRY MessageCallback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam) {
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
		PLOGE << "OGL: " << (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "") << " type = " << type << ", severity = " << severity << ", message = " << message;
	}
}
#endif

int main() {
	std::remove("latest.log");
	plog::init(plog::debug, "latest.log");
	plog::get()->addAppender(new plog::ColorConsoleAppender<plog::TxtFormatter>());
	PLOGI << "Logger initialized";

	auto* window = new Window();

#ifndef NDEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);
	PLOGI << "[OGL] Message debug callback created";
#endif

	PhysicsCommon physicsCommon;
	PhysicsWorld* world = physicsCommon.createPhysicsWorld();

	DebugRenderer& debugRenderer = world->getDebugRenderer();
	debugRenderer.setIsDebugItemDisplayed(DebugRenderer::DebugItem::COLLISION_SHAPE, true);
	world->setIsDebugRenderingEnabled(false);

#ifdef RMT_PROFILER
	Remotery* rmt;
	rmt_CreateGlobalInstance(&rmt);
	rmt_BindOpenGL();

	rmt_BeginCPUSample(Init, 0);
	PLOGI << "Profiler initialized";
#endif
	auto* shader = new Shader("main");
	auto* debugShader = new Shader("debug");
	auto lightPos = glm::vec3(-3.5f, 10.f, -1.5f);
	auto* shadows = new ShadowsCaster(4096, 4096, "depth", lightPos);

	auto* map = new Model("./data/meshes/map.obj", world, &physicsCommon, true);
	auto* player = new Model("./data/meshes/player.obj", world, &physicsCommon);
	player->rb->addCollider(physicsCommon.createCapsuleShape(1.f, 2.5f), Transform(Vector3(0, 1.2f, 0), Quaternion::identity()));
	player->rb->updateMassFromColliders();
	player->rb->updateLocalCenterOfMassFromColliders();
	auto* enemy = new Model("./data/meshes/player.obj", world, &physicsCommon);
	enemy->rb->addCollider(physicsCommon.createCapsuleShape(1.f, 2.5f), Transform(Vector3(0, 1.2f, 0), Quaternion::identity()));
	int id = 2;
	enemy->rb->setUserData(&id);
	auto* sniperRifle = new Model("./data/meshes/sniper.obj", world, &physicsCommon);
	BoxShape* sniperShape = physicsCommon.createBoxShape(Vector3(0.12f, 0.587f, 2.7f));
	Transform colliderOffset = Transform(Vector3(0, -0.08f, -0.36f), Quaternion::identity());
	sniperRifle->rb->addCollider(sniperShape, colliderOffset);
	sniperRifle->rb->setMass(1);
	player->rb->setAngularLockAxisFactor(Vector3::zero());
	PLOGI << "Loading models and physics successfully";

	auto* sniperTexture = new Texture("data/textures/sniper.png");
	auto* mapTexture = new Texture("data/textures/palette.png");
	PLOGI << "Loading textures successfully";

	auto* camera = new Camera(window->getWidthPtr(), window->getHeightPtr(), window->getRatioPtr());

	HUD* hud = new HUD(window);
	PLOGI << "Loading HUD (ImGui) successfully";

	Client* client = new Client();

	char* nickname = new char[64];
	char* ip = new char[16];

	memset(nickname, 0, 64);
	memset(ip, 0, 16);

	strcpy(nickname, "Player");
	strcpy(ip, "127.0.0.1");
#ifdef RMT_PROFILER
	rmt_EndCPUSample();
#endif

	while (window->update()) {
#ifdef RMT_PROFILER
		rmt_ScopedCPUSample(FrameUpdate, 0);
		rmt_ScopedOpenGLSample(FrameGPUUpdate);
#endif
		static bool debugWindow, menuWindow = true, wireframe, showControl = true;
		bool locked = false;
		static float shadowsDistance = 25.f;

		{
#ifdef RMT_PROFILER
			rmt_ScopedCPUSample(Update, 0);
#endif
			{
#ifdef RMT_PROFILER
				rmt_ScopedCPUSample(IOEvents, 0);
#endif
				camera->pollEvents(window, player->rb);

				if (glfwGetMouseButton(window->getId(), 0) == GLFW_PRESS) {
					Vector3 startPoint = player->rb->getTransform().getPosition();
					Vector3 endPoint(
						startPoint.x + std::cos(camera->rotation.y - 1.57f) * 50.f,
						startPoint.y + std::sin(camera->rotation.x) * 10.f,
						startPoint.z + std::sin(camera->rotation.y - 1.57f) * 50.f
					);
					Ray ray(startPoint, endPoint);
					RaycastInfo raycastInfo;
					locked = enemy->rb->raycast(ray, raycastInfo);
					if (locked) {
						enemy->rb->applyWorldForceAtWorldPosition(raycastInfo.worldNormal * -5, raycastInfo.worldPoint);
					}
				}

				if (window->isKeyPressed(GLFW_KEY_E)) {
					//Add force of drop
					Vector3 position(
						camera->position.x + std::cos(camera->rotation.y - 1.57f) * 2,
						camera->position.y,
						camera->position.z + std::sin(camera->rotation.y - 1.57f) * 2
					);
					sniperRifle->rb->setTransform(Transform(position, Quaternion::identity()));
					sniperRifle->rb->setLinearVelocity(Vector3::zero());
					sniperRifle->rb->setAngularVelocity(Vector3::zero());
					sniperRifle->rb->applyWorldForceAtCenterOfMass(
						Vector3(
							std::cos(camera->rotation.y - 1.57f) * 200,
							300,
							std::sin(camera->rotation.y - 1.57f) * 200)
					);
				}
			}
			{
#ifdef RMT_PROFILER
				rmt_ScopedCPUSample(Physics, 0);
#endif
				world->update(ImGui::GetIO().DeltaTime);
			}
		}

		{
#ifdef RMT_PROFILER
			rmt_ScopedCPUSample(Shadows, 0);
			rmt_ScopedOpenGLSample(ShadowsGPU);
#endif
			Shader* depth = shadows->begin(camera->position, shadowsDistance);
			depth->draw(sniperRifle);
			depth->draw(map);
			depth->draw(player);
			shadows->end();
		}

		{
#ifdef RMT_PROFILER
			rmt_ScopedCPUSample(Render, 0);
			rmt_ScopedOpenGLSample(RenderGPU);
#endif
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
			shader->draw(enemy);
			shader->unbind();
		}

		if (world->getIsDebugRenderingEnabled()) {
#ifdef RMT_PROFILER
			rmt_ScopedCPUSample(PhysicsRender, 0);
			rmt_ScopedOpenGLSample(PhysicsRenderGPU);
#endif
			{
				int verticesCount = debugRenderer.getNbTriangles() * 9;
				if (verticesCount > 0) {
					float* vertices = new float[verticesCount];
					for (int i = 0; i < debugRenderer.getNbTriangles(); i++) {
						DebugRenderer::DebugTriangle t = debugRenderer.getTriangles()[i];
						vertices[i * 9] = t.point1.x;
						vertices[i * 9 + 1] = t.point1.y;
						vertices[i * 9 + 2] = t.point1.z;

						vertices[i * 9 + 3] = t.point2.x;
						vertices[i * 9 + 4] = t.point2.y;
						vertices[i * 9 + 5] = t.point2.z;

						vertices[i * 9 + 6] = t.point3.x;
						vertices[i * 9 + 7] = t.point3.y;
						vertices[i * 9 + 8] = t.point3.z;
					}

					static unsigned int VAO = -1, VBO;

					if (VAO == -1) {
						glGenVertexArrays(1, &VAO);
						glBindVertexArray(VAO);

						glGenBuffers(1, &VBO);
						glBindBuffer(GL_ARRAY_BUFFER, VBO);
						glBufferData(GL_ARRAY_BUFFER, (int)(verticesCount * sizeof(float)), vertices, GL_DYNAMIC_DRAW);
						glEnableVertexAttribArray(0);
						glVertexAttribPointer(0, 3, GL_FLOAT, false,
							3 * sizeof(float), (void*)0);
					}
					else {
						glBindVertexArray(VAO);
						glBindBuffer(GL_ARRAY_BUFFER, VBO);
						glBufferData(GL_ARRAY_BUFFER, (int)(verticesCount * sizeof(float)), vertices, GL_DYNAMIC_DRAW);
					}

					debugShader->bind();
					debugShader->upload("proj", camera->getPerspective());
					debugShader->upload("view", camera->getView());
					glDrawArrays(GL_TRIANGLES, 0, verticesCount);
					debugShader->unbind();
					delete[] vertices;
				}
			}
		}

		{
#ifdef RMT_PROFILER
			rmt_ScopedCPUSample(HUDRender, 0);
			rmt_ScopedOpenGLSample(HUDRenderGPU);
#endif
			hud->begin();
			ImGui::SetNextWindowPos(ImVec2(20, 170), ImGuiCond_Once);

			if (ImGui::Begin("Debug", &debugWindow, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
#ifdef RMT_PROFILER
				rmt_ScopedCPUSample(HUD_Configuration , 0);
#endif
				ImGui::SliderFloat("Camera speed", &camera->speed, 0.01f, 10.f);
				ImGui::SliderFloat("Shadows distance", &shadowsDistance, 1.f, 50.f);
				ImGui::Separator();
				static bool vsync = true;
				if (ImGui::Checkbox("VSync", &vsync)) {
					window->setVsync(vsync);
				}
				static bool RMT_PROFILERDebugRender;
				if (ImGui::Checkbox("Debug render", &RMT_PROFILERDebugRender)) {
					world->setIsDebugRenderingEnabled(RMT_PROFILERDebugRender);
				}
				ImGui::Checkbox("Show wireframe", &wireframe);
				ImGui::Separator();
			}
			ImGui::End();

			ImGui::SetNextWindowPos(ImVec2(300, 20), ImGuiCond_Once);

			if (ImGui::Begin("Menu", &menuWindow, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
#ifdef RMT_PROFILER
				rmt_ScopedCPUSample(HUD_Menu, 0);
#endif
				ImGui::InputText("Nickname", nickname, 64, ImGuiInputTextFlags_NoHorizontalScroll);
				ImGui::Separator();
				ImGui::Spacing();
				const static std::regex ip_regex("(^((25[0-5]|(2[0-4]|1\\d|[1-9]|)\\d)\\.?\\b){4}$)");
				
				ImGui::InputText("IP", ip, 16, ImGuiInputTextFlags_NoHorizontalScroll);
				if (ImGui::Button(client->isConnected() ? "Disconnect" : "Connect") && (client->isConnected() || std::regex_match(ip, ip_regex))) {
					if (!client->isConnected()) {
						client->connectToHost(ip, 23403);
					}
					else {
						client->disconnectFromHost();
					}
				}
				ImGui::SameLine();
				if (std::regex_match(ip, ip_regex)) {
					ImGui::Text(client->getMessage());
				}
				else {
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "Thats not IP");
				}
				
				ImGui::Text("Connected: %s", client->isConnected() ? "yes" : "no");
				if (client->isConnected()) {
					if (ImGui::Button("Test packet")) {
						int data_length = strlen(nickname) + 1;
						char* data = new char[data_length];
						data[0] = strlen(nickname);
						memcpy(data + 1, nickname, strlen(nickname));
						client->sendPacket(PT_UPDATE, data, data_length);
					}
				}
			}
			ImGui::End();

			{
				ImGuiIO& io = ImGui::GetIO();
				ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Once);
				ImGui::SetNextWindowBgAlpha(0.35f);
				static bool _;
				if (ImGui::Begin("Debug overlay", &_, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
					ImGuiWindowFlags_NoSavedSettings |
					ImGuiWindowFlags_NoFocusOnAppearing |
					ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove)) {
#ifdef RMT_PROFILER
					rmt_ScopedCPUSample(HUD_DebugOverlay, 0);
#endif
					ImGui::Text("Screen: %dx%d", window->getWidth(), window->getHeight());
					ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
					ImGui::Text("Position: X: %.2f, Y: %.2f, Z: %.2f", camera->position.x, camera->position.y,
						camera->position.z);
					ImGui::Text("Rotation: X: %.2f, Y: %.2f", camera->rotation.x, camera->rotation.y);
					ImGui::Text("Hit: %s", locked ? "Yes" : "No");
				}
				ImGui::End();

				const float PAD = 20.0f;
				const ImGuiViewport* viewport = ImGui::GetMainViewport();
				{
#ifdef RMT_PROFILER
					rmt_ScopedCPUSample(HUD_Overlay, 0);
#endif
					ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
					ImVec2 work_size = viewport->WorkSize;
					ImVec2 window_pos, window_pos_pivot;
					window_pos.x = work_pos.x + work_size.x - PAD;
					window_pos.y = work_pos.y + work_size.y - PAD;
					window_pos_pivot.x = 1;
					window_pos_pivot.y = 1;
					ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
					ImGui::SetNextWindowBgAlpha(0.35f);

					if (ImGui::Begin("Ammo overlay", &_,
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

				if (ImGui::Begin("Controls", &_, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
					ImGuiWindowFlags_NoSavedSettings |
					ImGuiWindowFlags_NoFocusOnAppearing |
					ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove)) {
#ifdef RMT_PROFILER
					rmt_ScopedCPUSample(HUD_Controls, 0);
#endif
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
	}

	physicsCommon.destroyPhysicsWorld(world);
#ifdef RMT_PROFILER
	rmt_UnbindOpenGL();
	rmt_DestroyGlobalInstance(rmt);
#endif
	exit(EXIT_SUCCESS);
}