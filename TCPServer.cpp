#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <thread>
#include <conio.h>
#include <iostream>
#include <vector>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
const int MaxClients = 10;

SOCKET ClientSocket[MaxClients];

using namespace std;

void nospace(SOCKET &Sock)
{
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	int iResult = recv(Sock, recvbuf, recvbuflen, 0); //Принимаем ник клиента(для очистки принимающего буфера)
	printf("Server: client \"%s\" tried to log in. Not enough space.\n", recvbuf);

	iResult = send(Sock, "1", 2, 0); //Отвечаем, что нет мест

	closesocket(Sock); //Закрываем сокет
}

int ClientThread(int SocketNum)
{
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	bool Shutdown = false;
	string mes;
	cout << "Thread " << SocketNum << " started\n";

	int iResult = recv(ClientSocket[SocketNum], recvbuf, recvbuflen, 0); //Принимаем ник клиента
	iResult = send(ClientSocket[SocketNum], "0", 2, 0); //Отвечаем положительно

	string Nickname = recvbuf;
	mes = "Server: client \"" + Nickname + "\" logged in.\n";
	cout << mes;

	ZeroMemory(&recvbuf, sizeof(recvbuf));

	for (int i = 0; i < MaxClients; i++) //Рассылаем всем остальным
	{
		if (i != SocketNum && ClientSocket[i] != INVALID_SOCKET)
			iResult = send(ClientSocket[i], mes.c_str(), mes.length() + 1, 0);
	}

	

	while (!Shutdown)
	{
		iResult = recv(ClientSocket[SocketNum], recvbuf, recvbuflen, 0); //Принимаем строку от сервера
		if (iResult < 0) //Если потеряно соединение с клиентом
		{
			ZeroMemory(&recvbuf, sizeof(recvbuf));
			printf("Error: Lost connection with client \"%s\"\n", Nickname.c_str());
		}
		else printf("Client: \"%s\" Bytes received: %d\n", Nickname.c_str(), iResult);
		
		if (_stricmp("#exit", recvbuf) == 0 || iResult < 0)
		{
			Shutdown = true;
			mes = "Server: client \"" + Nickname + "\" logged out.\n";
			cout << mes;
			if (iResult >= 0) send(ClientSocket[SocketNum], "#done", sizeof("#done"), 0);
		}
		else
		{
			mes = "[" + Nickname + "]: " + recvbuf; //Добавляем ник к сообщению
		}

		ZeroMemory(&recvbuf, sizeof(recvbuf));

		for (int i = 0; i < MaxClients; i++) //Рассылаем всем остальным
		{
			if (i != SocketNum && ClientSocket[i] != INVALID_SOCKET)
				iResult = send(ClientSocket[i], mes.c_str(), mes.length() + 1, 0);
		}
	}

	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket[SocketNum], SD_SEND);
	// cleanup
	closesocket(ClientSocket[SocketNum]);
	ClientSocket[SocketNum] = INVALID_SOCKET;
	cout << "Thread " << SocketNum << " closed.\n";
	return 0;
}

int main()
{
	system("color F0");
	cout << "Starting server...\n";
	cout << "Maximum clients: " << MaxClients << endl;

	for (int i = 0; i < MaxClients; i++) ClientSocket[i] = INVALID_SOCKET;

	// Шаг 1 - подключение библиотеки версии 2.2

	WSADATA wsaData;
	int iResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
	// Выясняем, а поддерживается ли версия библиотеки 2.2
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		printf("Could not find a usable version of Winsock.dll\n");
		WSACleanup();
		_getch();
		return -1;
	}
	printf("The Winsock 2.2 dll was found ok\n");

	// Шаг 2 - открытие сокета

	SOCKET ListenSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = ::bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result); //Очищаем структруру result за ненадобностью 
	
	// Шаг 2 - открываем сокет для прослушивания

	cout << "Server started!\n";
	for (int i = 0; i < 80; i++) cout << "-"; //Просто проводим полоску

	iResult = listen(ListenSocket, MaxClients);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	SOCKET BufSocket = INVALID_SOCKET;
	vector<thread> thrs;

	while (1) //Цикл приема заявок на подключение
	{
		// Accept a client socket
		BufSocket = accept(ListenSocket, NULL, NULL);
		if (BufSocket == INVALID_SOCKET) 
		{
			printf("accept failed with error: %d\n", WSAGetLastError());
			continue;
		}

		for (int i = 0; i < MaxClients; i++)
		{
			if (ClientSocket[i] == INVALID_SOCKET)
			{
				ClientSocket[i] = BufSocket;
				thrs.emplace_back(thread(ClientThread, i));
				break;
			}
			if (i == MaxClients - 1) nospace(BufSocket);
		}
	}
	// No longer need server socket
	closesocket(ListenSocket);
	WSACleanup();

	return 0;
}