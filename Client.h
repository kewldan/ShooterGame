#ifndef SHOOTERGAME_CLIENT_H
#define SHOOTERGAME_CLIENT_H

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>

class Client {
	char* nickname;
	long key;
	SOCKET clientSocket;
	bool connected;
public:
	Client();
	void connectToHost(char* ip, int port);
	void sendBytes(char* bytes, int length);
	void reciveBytes(char* buffer, int length);
	bool isConnected();
};

#endif