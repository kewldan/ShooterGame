#include "Client.h"

Client::Client()
{
	WSADATA wsData;
	int err = WSAStartup(MAKEWORD(2, 2), &wsData);

	if (err != 0) {
		return;
	}

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (clientSocket == INVALID_SOCKET) {
		closesocket(clientSocket);
		WSACleanup();
		return;
	}
}

void Client::connectToHost(char* ip, int port)
{
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
	}
}

void Client::sendBytes(char* bytes, int length)
{
	send(clientSocket, bytes, length, 0);
}

void Client::reciveBytes(char* buffer, int length)
{
	recv(clientSocket, buffer, length, 0);
}

bool Client::isConnected()
{
	return connected;
}
