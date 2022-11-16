#include "Client.h"

Client::Client()
{
	connected = false;
	my_id = 0;
	clientSocket = 0;

	int err = WSAStartup(MAKEWORD(2, 2), &wsData);

	if (err != 0) {
		return;
	}
}

void Client::connectToHost(const char* ip, int port)
{
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (clientSocket == INVALID_SOCKET) {
		closesocket(clientSocket);
		WSACleanup();
		PLOGE << "Failed to open socket";
	}

	in_addr ip_to_num;
	inet_pton(AF_INET, ip, &ip_to_num);

	sockaddr_in servInfo;
	ZeroMemory(&servInfo, sizeof(servInfo));
	servInfo.sin_family = AF_INET;
	servInfo.sin_addr = ip_to_num;
	servInfo.sin_port = htons(port);

	int err = connect(clientSocket, (sockaddr*)&servInfo, sizeof(servInfo));
	if (err == 0) {
		connected = true;
		AppConsole::i->AddLog("[success] Connected to %s\n", ip);
	}
	else {
		AppConsole::i->AddLog("[error] Unable to connect to %s\n", ip);
	}
}

void Client::disconnectFromHost()
{
	shutdown(clientSocket, SD_SEND);
	connected = false;
	AppConsole::i->AddLog("[success] Disconnected");
}

void Client::sendBytes(char* bytes, int length)
{
	if (send(clientSocket, bytes, length, 0) == -1) {
		connected = false;
		AppConsole::i->AddLog("[error] Connection lost");
	}
}

int Client::reciveBytes(char* buffer, int length)
{
	return recv(clientSocket, buffer, length, 0);
}

BasicPacket* Client::recivePacket()
{
	char* packet_header_buffer = new char[4];
	int n = recv(clientSocket, packet_header_buffer, 4, 0);
	if (n == 4) {
		BasicPacket *packet = new BasicPacket();
		memcpy(&packet->length, packet_header_buffer, n);
		memcpy(&packet->type, packet_header_buffer + 2, n);

		packet->payload = new char[packet->length];

		int p = recv(clientSocket, packet->payload, packet->length, 0);
		if (p == packet->length) {
			return packet;
		}
		else {
			connected = false;
			AppConsole::i->AddLog("[error] Connection lost");
		}
	}
	else {
		connected = false;
		AppConsole::i->AddLog("[error] Connection lost");
	}
	return nullptr;
}

void Client::sendPacket(BasicPacket* packet)
{
	uint16_t packet_length = packet->length + 2;
	char* data = new char[packet_length + 2];

	memcpy(data, &packet_length, 2); //Service information
	memcpy(data + 2, &packet->type, 2);
	memcpy(data + 4, packet->payload, packet->length);

	sendBytes(data, packet_length + 2);
}

bool Client::isConnected()
{
	return connected;
}

unsigned long Client::getAvailbale()
{
	unsigned long d = 0;
	if (isConnected()) {
		ioctlsocket(clientSocket, FIONREAD, &d);
	}
	return d;
}

void Client::sendHandshake(char* nickname)
{
	BasicPacket* packet = new BasicPacket();
	packet->type = ClientPacketTypes::HANDSHAKE;
	packet->length = strlen(nickname) + 1;
	packet->payload = new char[packet->length];
	packet->payload[0] = strlen(nickname);
	memcpy(packet->payload + 1, nickname, strlen(nickname));
	sendPacket(packet);
}

void Client::sendUpdate(float x, float y, float z, float rx, float ry)
{
	BasicPacket* packet = new BasicPacket();
	packet->type = ClientPacketTypes::UPDATE;
	packet->length = sizeof(float) * 5;
	packet->payload = new char[packet->length];
	float* floats = new float[] {
		x, y, z, rx, ry
	};
	memcpy(packet->payload, floats, sizeof(float) * 5);

	sendPacket(packet);
}

void Client::sendMessage(char* message)
{
	BasicPacket* packet = new BasicPacket();
	packet->type = ClientPacketTypes::MESSAGE;
	packet->length = strlen(message);
	packet->payload = new char[packet->length];

	memcpy(packet->payload, message, strlen(message));

	sendPacket(packet);
}
