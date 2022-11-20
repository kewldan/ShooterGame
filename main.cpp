#include <reactphysics3d/reactphysics3d.h>

#define STB_IMAGE_IMPLEMENTATION

//#define RMT_PROFILER

#include "Shader.h"
#include <cstdlib>
#include "Window.h"

#include <plog/Log.h>
#include "plog/Initializers/RollingFileInitializer.h"
#include "plog/Formatters/FuncMessageFormatter.h"
#include "plog/Appenders/ColorConsoleAppender.h"
#include "Model.h"
#include "Camera.h"
#include "Texture.h"
#include "ShadowsCaster.h"
#include "Chat.h"

#include "imgui.h"
#include "HUD.h"
#include "Client.h"
#include "Remotery.h"
#include <regex>
#include <windows.h>
#include "Skybox.h"
#include "Minimap.h"
#include "GBuffer.h"


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


class EnemyModel : public Model {
	using Model::Model;
public:
	int id;
};

Window* window;
PhysicsCommon physicsCommon;
PhysicsWorld* world;
Shader* shader, * debugShader, * skyShader;
ShadowsCaster* shadows;
Model* map, * player, * sniperRifle;
Camera* camera;
HUD* hud;
Client* client;
char* nickname, * ip;
std::vector<EnemyModel*> enemies;
char enemies_count;
double lastUpdate;
Remotery* rmt;
Skybox* skybox;
Minimap* minimap;
std::map<int, char*> nicknames = std::map<int, char*>();
GBuffer* gBuffer;

const glm::vec3 lightPos(-3.5f, 10.f, -1.5f);
const std::regex ip_regex("(^((25[0-5]|(2[0-4]|1\\d|[1-9]|)\\d)\\.?\\b){4}$)");

Chat* Chat::i = nullptr;

bool wireframe, hasRifle = true, vsync = true, physicsDebugRender, console_open, castShadows, show_minimap = true;

struct Enemy {
	int id;
	float x, y, z, rx, ry;
};

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_E) {
			if (hasRifle) {
				sniperRifle->rb->setType(BodyType::DYNAMIC);
				BoxShape* sniperShape = physicsCommon.createBoxShape(Vector3(0.12f, 0.587f, 2.7f));
				Transform colliderOffset = Transform(Vector3(0, -0.08f, -0.36f), Quaternion::identity());
				sniperRifle->rb->addCollider(sniperShape, colliderOffset);
				sniperRifle->rb->setMass(1);
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
			else {
				sniperRifle->rb->removeCollider(sniperRifle->rb->getCollider(0));
				sniperRifle->rb->setType(BodyType::STATIC);
			}

			hasRifle ^= 1;
		}
		if (key == GLFW_KEY_BACKSPACE) {
			player->rb->setTransform(Transform::identity());
		}
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
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		Vector3 startPoint = player->rb->getTransform().getPosition();
		Vector3 endPoint(
			startPoint.x + std::cos(camera->rotation.y - 1.57f) * 50.f,
			startPoint.y + std::sin(camera->rotation.x) * 10.f,
			startPoint.z + std::sin(camera->rotation.y - 1.57f) * 50.f
		);
		Ray ray(startPoint, endPoint);
		RaycastInfo raycastInfo;

		for (int i = 0; i < enemies_count; i++) {
			EnemyModel* enemy = enemies[i];
			bool hit = enemy->rb->raycast(ray, raycastInfo);
			if (hit) {
				enemy->rb->applyWorldForceAtWorldPosition(raycastInfo.worldNormal * -200, raycastInfo.worldPoint);
			}
		}
	}
};

int main() {
	std::remove("latest.log");
	plog::init(plog::debug, "latest.log");

	window = new Window();

	glfwSetKeyCallback(window->getId(), key_callback);
	glfwSetMouseButtonCallback(window->getId(), mouse_button_callback);

	plog::get()->addAppender(new plog::ColorConsoleAppender<plog::FuncMessageFormatter>());

	PLOGI << "Logger initialized";

#ifndef NDEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);
	PLOGI << "[OGL] Message debug callback created";
#endif

	world = physicsCommon.createPhysicsWorld();

	DebugRenderer& debugRenderer = world->getDebugRenderer();
	debugRenderer.setIsDebugItemDisplayed(DebugRenderer::DebugItem::COLLISION_SHAPE, true);
	world->setIsDebugRenderingEnabled(false);

#ifdef RMT_PROFILER
	rmt_CreateGlobalInstance(&rmt);
	rmt_BindOpenGL();
	PLOGI << "Profiler initialized";
#endif

	shader = new Shader("./data/shaders/main");
	debugShader = new Shader("./data/shaders/debug");
	skyShader = new Shader("./data/shaders/sky");
	shadows = new ShadowsCaster(4096, 4096, "./data/shaders/depth", lightPos);

	map = new Model("./data/meshes/dust.obj", world, &physicsCommon, true);
	sniperRifle = new Model("./data/meshes/sniper.obj", world, &physicsCommon);
	sniperRifle->rb->setType(BodyType::STATIC);

	int nb = -1;
	MeshData* data = Model::loadMesh("./data/meshes/player.obj", &nb);

	CapsuleShape* playerShape = physicsCommon.createCapsuleShape(1.f, 2.5f);

	enemies = std::vector<EnemyModel*>();

	for (int i = 0; i < 16; i++) {
		EnemyModel* newEnemy = new EnemyModel(data, nb, world, &physicsCommon);
		newEnemy->rb->setTransform(Transform(Vector3(-9999, -9999, -9999), Quaternion::identity()));
		newEnemy->rb->addCollider(playerShape, Transform(Vector3(0, 1.2f, 0), Quaternion::identity()));
		newEnemy->rb->setType(BodyType::STATIC);
		enemies.push_back(newEnemy);
	}

	player = new Model(data, nb, world, &physicsCommon);
	player->rb->setTransform(Transform(Vector3(0, 20, 0), Quaternion::identity()));
	player->rb->addCollider(playerShape, Transform(Vector3(0, 1.2f, 0), Quaternion::identity()));
	player->rb->setAngularLockAxisFactor(Vector3::zero());

	delete[] data;

	skybox = new Skybox("data/textures/sky");

	hud = new HUD(window);
	Chat::i = new Chat();

	camera = new Camera(&window->width, &window->height, &window->ratio);

	minimap = new Minimap("./data/shaders/map", 1024, 1024, &camera->position, 60);

	gBuffer = new GBuffer("./data/shaders/pass1", "./data/shaders/pass2", &window->width, &window->height);

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

				camera->pollEvents(window, player->rb);

				if (hasRifle) {
					Vector3 newPos(camera->position.x + std::cos(camera->rotation.y) * 0.9f, camera->position.y - 0.4f, camera->position.z + std::sin(camera->rotation.y) * 0.9f);
					sniperRifle->rb->setTransform(Transform(newPos, Quaternion::fromEulerAngles(-camera->rotation.x, -camera->rotation.y, 0)));
				}
			}
			if (client->isConnected()) {
				rmt_ScopedCPUSample(Networking, 0);
				if (glfwGetTime() - lastUpdate > 0.02f) {
					Vector3 pos = player->rb->getTransform().getPosition();
					client->sendUpdate(pos.x, pos.y, pos.z, camera->rotation.x, camera->rotation.y);
					lastUpdate = glfwGetTime();
				}

				while (client->getAvailbale() > 4) {
					BasicPacket* packet = client->recivePacket();
					if (packet != nullptr) {
						if (packet->type == ServerPacketTypes::UPDATE) {
							enemies_count = packet->payload[0];

							for (int i = enemies_count; i < enemies.size(); i++) {
								enemies[i]->rb->setTransform(Transform(Vector3(-9999, -9999, -9999), Quaternion::identity()));
							}

							Enemy* e = new Enemy();
							for (int i = 0; i < enemies_count && i < enemies.size(); i++) {
								memcpy(e, packet->payload + 1 + i * sizeof(Enemy), sizeof(Enemy));

								EnemyModel* model = enemies[i];
								model->id = e->id;
								model->rb->setTransform(Transform(Vector3(e->x, e->y, e->z), Quaternion::identity()));
							}
							delete e;
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

							Chat::i->AddLog("[chat] [%s] %s", sender, message);
						}
						else if (packet->type == ServerPacketTypes::KICK) {
							char* reason = new char[packet->length + 1];
							memcpy(reason, packet->payload, packet->length);
							reason[packet->length] = 0;

							Chat::i->AddLog("[error] You kicked from server. Reason: %s\n", reason);
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
			{
				rmt_ScopedCPUSample(Physics, 0);
				world->update(ImGui::GetIO().DeltaTime);
			}
		}

		if (castShadows) {
			rmt_ScopedCPUSample(Shadows, 0);
			rmt_ScopedOpenGLSample(ShadowsGPU);
			Shader* depth = shadows->begin(camera->position, 25);
			depth->draw(sniperRifle);
			depth->draw(map);
			depth->draw(player);
			for (int i = 0; i < enemies_count && i < enemies.size(); i++) {
				depth->draw(enemies[i]);
			}
			shadows->end();
		}

		if (show_minimap) {
			rmt_ScopedCPUSample(MinimapUpdate, 0);
			rmt_ScopedOpenGLSample(MinimapGPU);
			Shader* mapShader = minimap->begin(camera->rotation.y);
			mapShader->upload("aTexture", 0);
			mapShader->upload("hasTexture", 1);
			glActiveTexture(GL_TEXTURE0);
			mapShader->draw(sniperRifle);
			mapShader->draw(map);
			mapShader->upload("hasTexture", 0);
			mapShader->draw(player);
			for (int i = 0; i < enemies_count && i < enemies.size(); i++) {
				mapShader->draw(enemies[i]);
			}
			minimap->end();
		}
			
		{
			rmt_ScopedCPUSample(GBufferUpdate, 0);
			rmt_ScopedOpenGLSample(GBufferGPU);
			Shader* pass1 = gBuffer->beginGeometryPass(camera->getPerspective(), camera->getView());
			pass1->upload("aTexture", 0);
			pass1->upload("hasTexture", 1);
			glActiveTexture(GL_TEXTURE0);
			pass1->draw(sniperRifle);
			pass1->draw(map);
			pass1->upload("hasTexture", 0);
			pass1->draw(player);
			for (int i = 0; i < enemies_count && i < enemies.size(); i++) {
				pass1->draw(enemies[i]);
			}
			gBuffer->endGeometryPass();
		}
		window->reset();

		{
			rmt_ScopedCPUSample(SkyboxUpdate, 0);
			rmt_ScopedOpenGLSample(SkyboxGPU);
			skybox->draw(skyShader, camera);
		}

		{

			rmt_ScopedCPUSample(Render, 0);
			rmt_ScopedOpenGLSample(RenderGPU);

			shader->bind();
			shader->upload("proj", camera->getPerspective());
			shader->upload("camera.transform", camera->getView());
			shader->upload("environment.sun_position", lightPos);
			shader->upload("displayWireframe", wireframe ? 1 : 0);
			shader->upload("viewportSize", glm::vec2(window->width, window->height));
			shader->upload("castShadows", castShadows ? 1 : 0);
			shader->upload("aTexture", 1);
			shader->upload("shadowMap", 0);
			shader->upload("lightSpaceMatrix", shadows->getLightSpaceMatrix());

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, shadows->getMap());
			for (int i = 0; i < enemies_count && i < enemies.size(); i++) {
				shader->draw(enemies[i]);
			}
			shader->upload("hasTexture", 1);
			glActiveTexture(GL_TEXTURE1);
			shader->draw(sniperRifle);
			shader->draw(map);
			shader->unbind();
		}

		if (world->getIsDebugRenderingEnabled()) {

			rmt_ScopedCPUSample(PhysicsRender, 0);
			rmt_ScopedOpenGLSample(PhysicsRenderGPU);

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

						glObjectLabelBuild(GL_VERTEX_ARRAY, VAO, "VAO", "Debug");

						glGenBuffers(1, &VBO);
						glBindBuffer(GL_ARRAY_BUFFER, VBO);
						glObjectLabelBuild(GL_BUFFER, VBO, "VBO", "Debug");
						glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(float), vertices, GL_DYNAMIC_DRAW);
						glEnableVertexAttribArray(0);
						glVertexAttribPointer(0, 3, GL_FLOAT, false,
							3 * sizeof(float), (void*)0);
					}
					else {
						glBindVertexArray(VAO);
						glBindBuffer(GL_ARRAY_BUFFER, VBO);
						glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(float), vertices, GL_DYNAMIC_DRAW);
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
			rmt_ScopedCPUSample(HUDRender, 0);
			rmt_ScopedOpenGLSample(HUDRenderGPU);

			hud->begin();
			ImGui::SetNextWindowPos(ImVec2(20, 170), ImGuiCond_Once);

			if (ImGui::Begin("Debug", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
				rmt_ScopedCPUSample(HUD_Configuration, 0);

				ImGui::SliderFloat("Camera speed", &camera->speed, 0.1f, 10.f, "%.1f");
				if (ImGui::Checkbox("VSync", &vsync)) {
					window->setVsync(vsync);
				}
				if (ImGui::Checkbox("Debug render", &physicsDebugRender)) {
					world->setIsDebugRenderingEnabled(physicsDebugRender);
				}
				ImGui::Checkbox("Show wireframe", &wireframe);
				ImGui::Checkbox("Cast shadows", &castShadows);
				ImGui::Checkbox("Show minimap", &show_minimap);
				if (show_minimap) {
					ImGui::Image((void*)(intptr_t)minimap->map, ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
				}
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

					ImGui::Text("https://github.com/kewldan/");
					ImGui::Text("Version: 55");
					ImGui::Text("");
					ImGui::Text("Screen: %dx%d", window->width, window->height);
					ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
					ImGui::Text("Position: X: %.1f, Y: %.1f, Z: %.1f", camera->position.x, camera->position.y,
						camera->position.z);
					ImGui::Text("Rotation: X: %.1f, Y: %.1f", camera->rotation.x, camera->rotation.y);
					if (client->isConnected()) {
						ImGui::Text("ID: %d", client->my_id);
						if (enemies_count > 0) {
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
						}
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
					Chat::i->Draw();
				}
				if (strlen(Chat::i->buffer) > 0) {
					if (client->isConnected()) {
						client->sendMessage(Chat::i->buffer);
					}
					memset(Chat::i->buffer, 0, 256);
				}
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