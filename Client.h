#ifndef SHOOTERGAME_CLIENT_H
#define SHOOTERGAME_CLIENT_H

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "md5.h"
#include "plog/Log.h"

#define PT_HANDSHAKE 1
#define PT_UPDATE 2

class Client {
	SOCKET clientSocket;
	bool connected;
	char* lastMessage;
public:
	Client();
	void connectToHost(const char* ip, int port);
	void disconnectFromHost();
	void sendBytes(char* bytes, int length);
	void reciveBytes(char* buffer, int length);
	void sendPacket(uint16_t type, char* payload, uint16_t length);
	bool isConnected();
	char* getMessage();
};

#endif