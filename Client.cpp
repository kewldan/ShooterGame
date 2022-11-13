#include "Client.h"

Client::Client()
{
	WSADATA wsData;
	int err = WSAStartup(MAKEWORD(2, 2), &wsData);

	if (err != 0) {
		return;
	}

	lastMessage = new char[32];
	strcpy(lastMessage, "No message");

	connected = false;
}

void Client::connectToHost(const char* ip, int port)
{
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (clientSocket == INVALID_SOCKET) {
		closesocket(clientSocket);
		WSACleanup();
		strcpy(lastMessage, "Failed to open socket");
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
		strcpy(lastMessage, "Connected");
	}
	else {
		strcpy(lastMessage, "Failed to connect");
	}
}

void Client::disconnectFromHost()
{
	shutdown(clientSocket, SD_SEND);
	connected = false;
	strcpy(lastMessage, "Disconnected");
}

void Client::sendBytes(char* bytes, int length)
{
	if (send(clientSocket, bytes, length, 0) == -1) {
		connected = false;
		strcpy(lastMessage, "Force disconnected");
	}
}

int Client::reciveBytes(char* buffer, int length)
{
	return recv(clientSocket, buffer, length, 0);
}

void Client::sendPacket(uint16_t type, char* payload, uint16_t length)
{
	uint16_t packet_length = length + 18;
	uint16_t data_length = packet_length + 2;
	char* data = new char[data_length];
	memcpy(data, &packet_length, 2);
	char* to_hash = new char[length + 1];
	memcpy(to_hash, payload, length);
	to_hash[length] = 0;
	MD5 md5(to_hash);
	char* hash = md5.getBytes();
	memcpy(data + 2, hash, 16);
	memcpy(data + 18, &type, 2);
	memcpy(data + 20, payload, length);

	sendBytes(data, data_length);
}

bool Client::isConnected()
{
	return connected;
}

char* Client::getMessage()
{
	return lastMessage;
}

void Client::sendHandshake(char* nickname)
{
	int data_length = strlen(nickname) + 1;
	char* data = new char[data_length];
	data[0] = strlen(nickname);
	memcpy(data + 1, nickname, strlen(nickname));
	sendPacket(ClientPacketTypes::HANDSHAKE, data, data_length);
}

void Client::sendUpdate(float x, float y, float z, float rx, float ry)
{
	int data_length = sizeof(float) * 5;
	char* data = new char[data_length];
	float* floats = new float[] {
		x, y, z, rx, ry
	};
	memcpy(data, floats, sizeof(float) * 5);

	sendPacket(ClientPacketTypes::UPDATE, data, data_length);
}
