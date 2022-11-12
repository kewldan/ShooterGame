#ifndef SHOOTERGAME_CLIENT_H
#define SHOOTERGAME_CLIENT_H

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "md5.h"
#include "plog/Log.h"

class ClientPacketTypes {
public:
	const static uint16_t HANDSHAKE = 2;
	const static uint16_t UPDATE = 4;
	const static uint16_t EXIT = 8;
};


class ServerPacketTypes {
public:
	const static uint16_t HANDSHAKE = 1;
	const static uint16_t UPDATE = 3;
	const static uint16_t KICK = 5;
};

class Client {
	SOCKET clientSocket;
	bool connected;
	char* lastMessage;
public:
	Client();
	void connectToHost(const char* ip, int port);
	void disconnectFromHost();
	void sendBytes(char* bytes, int length);
	int reciveBytes(char* buffer, int length);
	void sendPacket(uint16_t type, char* payload, uint16_t length);
	bool isConnected();
	char* getMessage();
};

#endif