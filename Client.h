#ifndef SHOOTERGAME_CLIENT_H
#define SHOOTERGAME_CLIENT_H

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "plog/Log.h"
#include "Chat.h"

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

class Client {
	bool connected;
	WSADATA wsData;
	void sendBytes(char* bytes, int length);
	int reciveBytes(char* buffer, int length);
public:
	SOCKET clientSocket;
	unsigned int my_id;

	Client();
	void connectToHost(const char* ip, int port);
	void disconnectFromHost();
	bool isConnected();
	unsigned long getAvailbale();

	BasicPacket* recivePacket();
	void sendPacket(BasicPacket* packet);
	void sendHandshake(char* nickname);
	void sendUpdate(float x, float y, float z, float rx, float ry);
	void sendMessage(char* message);
	void sendPlayerRequest(int id);
};

#endif