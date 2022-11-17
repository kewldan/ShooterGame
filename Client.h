#ifndef SHOOTERGAME_CLIENT_H
#define SHOOTERGAME_CLIENT_H

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "plog/Log.h"
#include "Console.h"
#include <GLFW/glfw3.h>

class ClientPacketTypes {
public:
	const static uint16_t HANDSHAKE = 2;
	const static uint16_t UPDATE = 4;
	const static uint16_t MESSAGE = 6;
	const static uint16_t GET_PLAYER = 8;
};


class ServerPacketTypes {
public:
	const static uint16_t HANDSHAKE = 1;
	const static uint16_t UPDATE = 3;
	const static uint16_t MESSAGE = 5;
	const static uint16_t KICK = 7;
	const static uint16_t PLAYER_INFO = 9;
};

struct BasicPacket {
	uint16_t length, type;
	char* payload;
};

struct PacketsInfo {
	unsigned int sent, previousSecond;
	double lastUpdate;
};

class Client {
	bool connected;
	WSADATA wsData;
	void sendBytes(char* bytes, int length);
	int reciveBytes(char* buffer, int length);
	PacketsInfo outcoming, incoming;
public:
	SOCKET clientSocket;
	unsigned int my_id;

	Client();
	void connectToHost(const char* ip, int port);
	void disconnectFromHost();
	bool isConnected();
	unsigned long getAvailbale();
	PacketsInfo* getIncoming();
	PacketsInfo* getOutcoming();

	BasicPacket* recivePacket();
	void sendPacket(BasicPacket* packet);
	void sendHandshake(char* nickname);
	void sendUpdate(float x, float y, float z, float rx, float ry);
	void sendMessage(char* message);
	void sendPlayerRequest(int id);
};

#endif