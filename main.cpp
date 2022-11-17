//#define RMT_PROFILER

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

#include "bullet/btBulletDynamicsCommon.h"

#include "imgui.h"
#include "HUD.h"
#include "Client.h"
#include "Remotery.h"
#include <regex>
#include <windows.h>
#include "Console.h"

#ifndef NDEBUG
void GLAPIENTRY MessageCallback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam) {
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
		PLOGE << "OGL: " << " type = " << type << ", severity = " << severity << ", message = " << message;
	}
}
#endif

Window* window;
Shader* shader, * debugShader;
ShadowsCaster* shadows;
Model* map, * player, * sniperRifle;
Texture* sniperTexture, * mapTexture;
Camera* camera;
HUD* hud;
Client* client;

btDefaultCollisionConfiguration* collisionConfiguration;
btCollisionDispatcher* dispatcher;
btBroadphaseInterface* overlappingPairCache;
btSequentialImpulseConstraintSolver* solver;
btDiscreteDynamicsWorld* dynamicsWorld;

char* nickname, * ip;
char enemies_count;
double lastUpdate;
Remotery* rmt;
AppConsole* console;
std::map<int, char*> nicknames = std::map<int, char*>();

const glm::vec3 lightPos(-3.5f, 10.f, -1.5f);
const std::regex ip_regex("(^((25[0-5]|(2[0-4]|1\\d|[1-9]|)\\d)\\.?\\b){4}$)");

AppConsole* AppConsole::i = nullptr;

bool wireframe, hasRifle = true, vsync = true, console_open;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_ESCAPE) {
			exit(0);
		}
		if (key == GLFW_KEY_GRAVE_ACCENT) {
			console_open ^= 1;
		}
	}
};


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		if (action >= GLFW_PRESS) {
			camera->hFov = 25;
		}
		else {
			camera->hFov = 60;
		}
	}
};

int main() {
	std::remove("latest.log");
	plog::init(plog::debug, "latest.log");

	window = new Window();

	glfwSetKeyCallback(window->getId(), key_callback);
	glfwSetMouseButtonCallback(window->getId(), mouse_button_callback);

	plog::get()->addAppender(new plog::ColorConsoleAppender<plog::TxtFormatter>());

	PLOGI << "Logger initialized";

#ifndef NDEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);
	PLOGI << "[OGL] Message debug callback created";
#endif

#ifdef RMT_PROFILER
	rmt_CreateGlobalInstance(&rmt);
	rmt_BindOpenGL();
	PLOGI << "Profiler initialized";
#endif

	shader = new Shader("./data/shaders/main");
	debugShader = new Shader("./data/shaders/debug");
	shadows = new ShadowsCaster(4096, 4096, "./data/shaders/depth", lightPos);

	map = new Model("./data/meshes/map.obj");
	sniperRifle = new Model("./data/meshes/sniper.obj");

	std::vector<float> output = std::vector<float>(), vertices = std::vector<float>();
	std::vector<int> indices = std::vector<int>();
	Model::loadMesh("./data/meshes/player.obj", &indices, &vertices, &output);

	PLOGI << "Loading models and physics successfully";

	sniperTexture = new Texture("data/textures/sniper.png");
	mapTexture = new Texture("data/textures/palette.png");
	PLOGI << "Loading textures successfully";

	collisionConfiguration = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);
	overlappingPairCache = new btDbvtBroadphase();
	solver = new btSequentialImpulseConstraintSolver();
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,
		overlappingPairCache, solver, collisionConfiguration);
	dynamicsWorld->setGravity(btVector3(0, -9.8f, 0));

	btBoxShape* box = new btBoxShape(btVector3(0, 0, 0));
	btRigidBody* body = new btRigidBody(0, new btDefaultMotionState(btTransform(btQuaternion::getIdentity(), btVector3(0, 0 ,0))), box);

	exit(0);

	PLOGI << "Bullet initialized successfully";

	hud = new HUD(window);
	console = new AppConsole();
	AppConsole::i = console;

	camera = new Camera(&window->width, &window->height, &window->ratio);

	client = new Client();
	nickname = new char[64];
	ip = new char[16];
	DWORD username_len = 64;
	GetUserName(nickname, &username_len);

	strcpy(ip, "127.0.0.1");
	while (window->update()) {
		rmt_ScopedCPUSample(FrameUpdate, 0);
		rmt_ScopedOpenGLSample(FrameGPUUpdate);
		{
			rmt_ScopedCPUSample(Update, 0);
			{
				rmt_ScopedCPUSample(IOEvents, 0);

				camera->pollEvents(window);
			}
			if (client->isConnected()) {
				rmt_ScopedCPUSample(Networking, 0);
				if (glfwGetTime() - lastUpdate > 0.02f) {
					//client->sendUpdate(pos.x, pos.y, pos.z, camera->rotation.x, camera->rotation.y);
					lastUpdate = glfwGetTime();
				}

				while (client->getAvailbale() > 4) {
					BasicPacket* packet = client->recivePacket();
					if (packet != nullptr) {
						if (packet->type == ServerPacketTypes::UPDATE) {
							enemies_count = packet->payload[0];

							/*for (int i = enemies_count; i < enemies.size(); i++) {
								//enemies[i]->rb->setTransform(Transform(Vector3(-9999, -9999, -9999), Quaternion::identity()));
							}

							Enemy* e = new Enemy();
							for (int i = 0; i < enemies_count && i < enemies.size(); i++) {
								memcpy(e, packet->payload + 1 + i * sizeof(Enemy), sizeof(Enemy));

								EnemyModel* model = enemies[i];
								model->id = e->id;
								//model->rb->setTransform(Transform(Vector3(e->x, e->y, e->z), Quaternion::identity()));
							}
							delete e;*/
						}
						else if (packet->type == ServerPacketTypes::HANDSHAKE) {
							memcpy(&client->my_id, packet->payload, 4);
						}
						else if (packet->type == ServerPacketTypes::MESSAGE) {
							char sender_length, message_length;

							memcpy(&sender_length, packet->payload, 1);
							char* sender = new char[sender_length + 1];
							memcpy(sender, packet->payload + 1, sender_length);
							sender[sender_length] = 0;

							memcpy(&message_length, packet->payload + 1 + sender_length, 1);
							char* message = new char[message_length + 1];
							memcpy(message, packet->payload + 2 + sender_length, message_length);
							message[message_length] = 0;

							console->AddLog("[chat] [%s] %s", sender, message);
						}
						else if (packet->type == ServerPacketTypes::KICK) {
							char* reason = new char[packet->length + 1];
							memcpy(reason, packet->payload, packet->length);
							reason[packet->length] = 0;

							console->AddLog("[error] You kicked from server. Reason: %s\n", reason);
						}
						else if (packet->type == ServerPacketTypes::PLAYER_INFO) {
							char* nickname = new char[packet->length - 4 + 1];
							memcpy(nickname, packet->payload + 4, packet->length);
							nickname[packet->length - 4] = 0;

							int id = 0;
							memcpy(&id, packet->payload, 4);

							nicknames[id] = nickname;
						}
					}
				}
			}
		}
		{
			rmt_ScopedCPUSample(Shadows, 0);
			rmt_ScopedOpenGLSample(ShadowsGPU);
			Shader* depth = shadows->begin(camera->position, 25);
			depth->draw(sniperRifle);
			depth->draw(map);
			depth->draw(player);
			shadows->end();
		}
		{

			rmt_ScopedCPUSample(Render, 0);
			rmt_ScopedOpenGLSample(RenderGPU);

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
			shader->unbind();
		}

		{
			rmt_ScopedCPUSample(HUDRender, 0);
			rmt_ScopedOpenGLSample(HUDRenderGPU);

			hud->begin();
			ImGui::SetNextWindowPos(ImVec2(20, 170), ImGuiCond_Once);

			if (ImGui::Begin("Debug", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
				rmt_ScopedCPUSample(HUD_Configuration, 0);

				ImGui::SliderFloat("Camera speed", &camera->speed, 0.01f, 10.f);
				ImGui::Separator();
				if (ImGui::Checkbox("VSync", &vsync)) {
					window->setVsync(vsync);
				}
				ImGui::Checkbox("Show wireframe", &wireframe);
				ImGui::Separator();
			}
			ImGui::End();

			ImGui::SetNextWindowPos(ImVec2(300, 20), ImGuiCond_Once);

			if (ImGui::Begin("Menu", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {

				rmt_ScopedCPUSample(HUD_Menu, 0);

				if (client->isConnected()) {
					ImGui::BeginDisabled();
				}
				ImGui::InputText("IP", ip, 16, ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_CharsScientific);
				ImGui::InputText("Nickname", nickname, 32, ImGuiInputTextFlags_NoHorizontalScroll);
				if (client->isConnected()) {
					ImGui::EndDisabled();
				}

				if (!std::regex_match(ip, ip_regex) && !client->isConnected()) {
					ImGui::BeginDisabled();
				}

				if (ImGui::Button(client->isConnected() ? "Disconnect" : "Connect", ImVec2(270, 20))) {
					if (!client->isConnected()) {
						client->connectToHost(ip, 23403);
						if (client->isConnected()) {
							client->sendHandshake(nickname);
						}
					}
					else {
						client->disconnectFromHost();
					}
				}

				if (!std::regex_match(ip, ip_regex) && !client->isConnected()) {
					ImGui::EndDisabled();
				}
			}
			ImGui::End();

			{
				ImGuiIO& io = ImGui::GetIO();
				ImGui::SetNextWindowPos(ImVec2(15, 15), ImGuiCond_Once);
				ImGui::SetNextWindowBgAlpha(0.35f);
				if (ImGui::Begin("##Debug overlay", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
					ImGuiWindowFlags_NoSavedSettings |
					ImGuiWindowFlags_NoFocusOnAppearing |
					ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove)) {

					rmt_ScopedCPUSample(HUD_DebugOverlay, 0);

					ImGui::Text("Screen: %dx%d", window->width, window->height);
					ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
					ImGui::Text("Position: X: %.1f, Y: %.1f, Z: %.1f", camera->position.x, camera->position.y,
						camera->position.z);
					ImGui::Text("Rotation: X: %.1f, Y: %.1f", camera->rotation.x, camera->rotation.y);
					if (client->isConnected()) {
						ImGui::Text("ID: %d", client->my_id);
						ImGui::Text("Packets: %d/s in, %d/s out", client->getIncoming()->previousSecond, client->getOutcoming()->previousSecond);
						/*if (enemies_count > 0) {
							ImGui::Text("Player list (K/D):");
							for (int i = 0; i < enemies_count; i++) {
								if (nicknames.contains(enemies[i]->id)) {
									if (strlen(nicknames[enemies[i]->id]) > 0) {
										ImGui::Text("%d. %s (0/0)", i + 1, nicknames[enemies[i]->id]);
									}
									else {
										ImGui::Text("%d. Unknown (0/0)", i + 1);
									}
								}
								else {
									client->sendPlayerRequest(enemies[i]->id);
									nicknames[enemies[i]->id] = new char[1];
									nicknames[enemies[i]->id][0] = 0;
									ImGui::Text("%d. Unknown (0/0)", i + 1);
								}
							}
						}*/
					}
				}
				ImGui::End();

				const ImGuiViewport* viewport = ImGui::GetMainViewport();
				{
					rmt_ScopedCPUSample(HUD_Overlay, 0);
					ImGui::SetNextWindowPos({
						viewport->WorkPos.x + viewport->WorkSize.x - 15.f,
						viewport->WorkPos.y + viewport->WorkSize.y - 15.f
						}, ImGuiCond_Always, { 1, 1 });
					ImGui::SetNextWindowBgAlpha(0.f);
					ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

					if (ImGui::Begin("##Ammo overlay", NULL,
						ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
						ImGuiWindowFlags_NoSavedSettings |
						ImGuiWindowFlags_NoFocusOnAppearing |
						ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove |
						ImGuiWindowFlags_NoInputs)) {
						ImGui::Text("Health: %d / %d", 80, 100);
						ImGui::Text("");
						ImGui::Text("Name: %s", "AK-47");
						ImGui::Text("Ammo: %d / %d", 17, 30);
					}
					ImGui::End();
					ImGui::PopStyleVar();
				}

				if (console_open) {
					console->Draw();
				}
				if (strlen(console->buffer) > 0) {
					if (client->isConnected()) {
						client->sendMessage(console->buffer);
					}
					memset(console->buffer, 0, 256);
				}
			}
			hud->end();
		}
	}

	//physicsCommon.destroyPhysicsWorld(world);

#ifdef RMT_PROFILER
	rmt_UnbindOpenGL();
	rmt_DestroyGlobalInstance(rmt);
#endif

	exit(EXIT_SUCCESS);
}